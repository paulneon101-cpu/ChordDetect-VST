#include "PitchDetector.h"

PitchDetector::PitchDetector()
    : fft       (FFT_ORDER),
      windowing (FFT_SIZE, juce::dsp::WindowingFunction<float>::hann)
{
    fftData   .resize(FFT_SIZE * 2, 0.0f);
    magnitudes.resize(FFT_SIZE / 2, 0.0f);
    fifo      .resize(FFT_SIZE,     0.0f);
}

void PitchDetector::prepare(double sr, int /*blockSize*/)
{
    sampleRate = sr;
    fifoIndex  = 0;
    std::fill(fifo.begin(),    fifo.end(),    0.0f);
    std::fill(fftData.begin(), fftData.end(), 0.0f);
    juce::ScopedLock sl(lock);
    activeNotes.clear();
}

void PitchDetector::processBlock(const juce::AudioBuffer<float>& buffer)
{
    // Mix down to mono for analysis
    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        float sample = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            sample += buffer.getReadPointer(ch)[i];
        sample /= (float)numChannels;

        fifo[fifoIndex++] = sample;

        if (fifoIndex == FFT_SIZE)
        {
            fifoIndex = 0;
            std::copy(fifo.begin(), fifo.end(), fftData.begin());
            runFFT();
        }
    }
}

void PitchDetector::runFFT()
{
    windowing.multiplyWithWindowingTable(fftData.data(), FFT_SIZE);
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // Normalize
    float maxMag = *std::max_element(fftData.begin(), fftData.begin() + FFT_SIZE / 2);
    if (maxMag < 1e-9f)
    {
        juce::ScopedLock sl(lock);
        activeNotes.clear();
        return;
    }

    for (int i = 0; i < FFT_SIZE / 2; ++i)
        magnitudes[i] = fftData[i] / maxMag;

    std::set<int> found;
    const float   threshold = 0.12f;

    // Peak picking: local maxima above threshold, ignoring harmonics
    for (int i = 3; i < FFT_SIZE / 2 - 3; ++i)
    {
        const float m = magnitudes[i];
        if (m < threshold) continue;

        // Must be local maximum
        if (m <= magnitudes[i - 1] || m <= magnitudes[i + 1]) continue;
        if (m <= magnitudes[i - 2] || m <= magnitudes[i + 2]) continue;

        const float freq = (float)i * (float)sampleRate / (float)FFT_SIZE;
        if (freq < 27.5f || freq > 4186.0f) continue;   // piano range A0–C8

        const int midi = frequencyToMidiNote(freq);
        if (midi >= 21 && midi <= 108)
            found.insert(midi % 12);                      // store as pitch class
    }

    juce::ScopedLock sl(lock);
    activeNotes = found;
}

int PitchDetector::frequencyToMidiNote(float freq) const
{
    return (int)std::round(69.0f + 12.0f * std::log2(freq / 440.0f));
}

std::set<int> PitchDetector::getActiveNotes() const
{
    juce::ScopedLock sl(lock);
    return activeNotes;
}
