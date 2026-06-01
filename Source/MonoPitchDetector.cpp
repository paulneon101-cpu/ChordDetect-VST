#include "MonoPitchDetector.h"

MonoPitchDetector::MonoPitchDetector()
{
    fifo  .resize (FRAME_SIZE, 0.0f);
    yinBuf.resize (FRAME_SIZE / 2, 0.0f);
}

void MonoPitchDetector::prepare (double sr, int /*blockSize*/)
{
    sampleRate = sr;
    fifoIndex  = 0;
    std::fill (fifo.begin(), fifo.end(), 0.0f);
    juce::ScopedLock sl (lock);
    currentNote       = -1;
    currentConfidence = 0.0f;
}

void MonoPitchDetector::setEnabled (bool e)
{
    enabled = e;
    if (!e)
    {
        juce::ScopedLock sl (lock);
        currentNote = -1;
    }
}

void MonoPitchDetector::setSensitivity (float s)
{
    sensitivity = juce::jlimit (0.0f, 1.0f, s);
}

bool MonoPitchDetector::isEnabled() const { return enabled; }

int MonoPitchDetector::getCurrentMidiNote() const
{
    juce::ScopedLock sl (lock);
    return currentNote;
}

float MonoPitchDetector::getConfidence() const
{
    juce::ScopedLock sl (lock);
    return currentConfidence;
}

//==============================================================================
void MonoPitchDetector::processBlock (const juce::AudioBuffer<float>& buffer)
{
    if (!enabled) return;

    // Use first channel (mic typically mono)
    const float* src      = buffer.getReadPointer (0);
    const int    numSamps = buffer.getNumSamples();

    for (int i = 0; i < numSamps; ++i)
    {
        fifo[fifoIndex++] = src[i];

        if (fifoIndex == FRAME_SIZE)
        {
            fifoIndex = 0;

            float freq = detectPitch (fifo.data(), FRAME_SIZE);

            juce::ScopedLock sl (lock);
            if (freq > 0.0f)
            {
                float midiFloat = 69.0f + 12.0f * std::log2 (freq / 440.0f);
                currentNote     = juce::jlimit (0, 127, (int)std::round (midiFloat));
                // Confidence inversely proportional to yin minimum value
                currentConfidence = juce::jlimit (0.0f, 1.0f, 1.0f - currentConfidence);
            }
            else
            {
                currentNote       = -1;
                currentConfidence = 0.0f;
            }
        }
    }
}

//==============================================================================
// YIN algorithm — de Cheveigne & Kawahara (2002)
float MonoPitchDetector::detectPitch (const float* samples, int numSamples)
{
    const int halfLen = numSamples / 2;

    // Map sensitivity (0-1) -> YIN threshold (0.25 loose .. 0.05 strict)
    const float threshold = 0.25f - sensitivity * 0.20f;

    // 1. Difference function
    yinBuf[0] = 1.0f;
    float runningSum = 0.0f;

    for (int tau = 1; tau < halfLen; ++tau)
    {
        float delta = 0.0f;
        for (int j = 0; j < halfLen; ++j)
        {
            float d = samples[j] - samples[j + tau];
            delta  += d * d;
        }

        // 2. Cumulative mean normalized difference function
        runningSum += delta;
        yinBuf[tau] = (runningSum > 0.0f)
                    ? delta * (float)tau / runningSum
                    : 0.0f;
    }

    // 3. Absolute threshold — find first tau below threshold
    int tau = 2;
    while (tau < halfLen)
    {
        if (yinBuf[tau] < threshold)
        {
            // Walk to local minimum
            while (tau + 1 < halfLen && yinBuf[tau + 1] < yinBuf[tau])
                ++tau;

            // 4. Parabolic interpolation for sub-sample accuracy
            float s0 = (tau > 0)           ? yinBuf[tau - 1] : yinBuf[tau];
            float s1 =                        yinBuf[tau];
            float s2 = (tau < halfLen - 1) ? yinBuf[tau + 1] : yinBuf[tau];

            float denom     = 2.0f * (2.0f * s1 - s2 - s0);
            float betterTau = (denom != 0.0f)
                            ? (float)tau + (s2 - s0) / denom
                            : (float)tau;

            // Clamp to human voice range 60 Hz – 1500 Hz
            float freq = (float)sampleRate / betterTau;
            if (freq >= 60.0f && freq <= 1500.0f)
                return freq;

            return -1.0f;
        }
        ++tau;
    }

    return -1.0f;
}
