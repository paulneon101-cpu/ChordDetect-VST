#include "PitchDetector.h"

PitchDetector::PitchDetector()
{
    initFFT();
}

//==============================================================================
void PitchDetector::initFFT()
{
    const int fftSize = 1 << fftOrder;

    fft      = std::make_unique<juce::dsp::FFT> (fftOrder);
    windowing = std::make_unique<juce::dsp::WindowingFunction<float>> (
                    (size_t)fftSize, juce::dsp::WindowingFunction<float>::hann);

    fftData   .assign (fftSize * 2, 0.0f);
    magnitudes.assign (fftSize / 2, 0.0f);
    fifo      .assign (fftSize,     0.0f);
    fifoIndex = 0;

    juce::ScopedLock sl (lock);
    activeNotes.clear();
}

void PitchDetector::setFFTOrder (int order)
{
    jassert (order >= 9 && order <= 13);
    if (order == fftOrder) return;
    fftOrder = order;
    initFFT();
}

void PitchDetector::setPitchCorrection (float percent)
{
    pitchCorrectionPercent = juce::jlimit (0.0f, 100.0f, percent);
}

//==============================================================================
void PitchDetector::prepare (double sr, int /*blockSize*/)
{
    sampleRate = sr;
    initFFT();
}

void PitchDetector::processBlock (const juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();
    const int fftSize     = 1 << fftOrder;

    for (int i = 0; i < numSamples; ++i)
    {
        float sample = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            sample += buffer.getReadPointer (ch)[i];
        sample /= (float)numChannels;

        fifo[fifoIndex++] = sample;

        if (fifoIndex == fftSize)
        {
            fifoIndex = 0;
            std::copy (fifo.begin(), fifo.end(), fftData.begin());
            runFFT();
        }
    }
}

void PitchDetector::runFFT()
{
    const int fftSize = 1 << fftOrder;

    windowing->multiplyWithWindowingTable (fftData.data(), (size_t)fftSize);
    fft->performFrequencyOnlyForwardTransform (fftData.data());

    float maxMag = *std::max_element (fftData.begin(), fftData.begin() + fftSize / 2);
    if (maxMag < 1e-9f)
    {
        juce::ScopedLock sl (lock);
        activeNotes.clear();
        return;
    }

    for (int i = 0; i < fftSize / 2; ++i)
        magnitudes[i] = fftData[i] / maxMag;

    // Pitch correction: map percent → max allowed cents deviation from equal temperament
    // 0% → 50 ¢ (very permissive), 100% → 2 ¢ (strict)
    const float maxCents = 50.0f - (pitchCorrectionPercent / 100.0f) * 48.0f;

    std::set<int> found;
    const float   threshold = 0.12f;

    for (int i = 3; i < fftSize / 2 - 3; ++i)
    {
        const float m = magnitudes[i];
        if (m < threshold) continue;
        if (m <= magnitudes[i - 1] || m <= magnitudes[i + 1]) continue;
        if (m <= magnitudes[i - 2] || m <= magnitudes[i + 2]) continue;

        const float freq = (float)i * (float)sampleRate / (float)fftSize;
        if (freq < 27.5f || freq > 4186.0f) continue;

        // Cent deviation from nearest equal-tempered pitch
        const float midiFloat   = 69.0f + 12.0f * std::log2 (freq / 440.0f);
        const float nearestMidi = std::round (midiFloat);
        const float cents       = std::abs (midiFloat - nearestMidi) * 100.0f;

        if (cents > maxCents) continue;  // pitch-correction gate

        const int midi = (int)nearestMidi;
        if (midi >= 21 && midi <= 108)
            found.insert (midi % 12);
    }

    juce::ScopedLock sl (lock);
    activeNotes = found;
}

int PitchDetector::frequencyToMidiNote (float freq) const
{
    return (int)std::round (69.0f + 12.0f * std::log2 (freq / 440.0f));
}

std::set<int> PitchDetector::getActiveNotes() const
{
    juce::ScopedLock sl (lock);
    return activeNotes;
}
