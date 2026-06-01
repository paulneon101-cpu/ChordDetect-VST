#include "AudioEffectChain.h"

void AudioEffectChain::prepare (double sr, int maxBlock, int numCh)
{
    sampleRate  = sr;
    numChannels = numCh;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sr;
    spec.maximumBlockSize = (juce::uint32)maxBlock;
    spec.numChannels      = (juce::uint32)numCh;

    lowShelf .prepare (spec);
    midPeak  .prepare (spec);
    highShelf.prepare (spec);
    reverb   .prepare (spec);

    applyEQCoeffs (0.f, 80.f, 0.f, 1000.f, 0.f, 8000.f);

    juce::dsp::Reverb::Parameters rp;
    rp.roomSize = 0.3f;  rp.wetLevel = 0.0f;  rp.dryLevel = 1.0f;
    rp.damping  = 0.5f;  rp.width    = 1.0f;
    reverb.setParameters (rp);

    delayBufL.assign (MAX_DELAY_SAMPLES, 0.0f);
    delayBufR.assign (MAX_DELAY_SAMPLES, 0.0f);
    delayWrite = 0;
}

//==============================================================================
void AudioEffectChain::applyEQCoeffs (float lowDB,  float lowHz,
                                       float midDB,  float midHz,
                                       float highDB, float highHz)
{
    const double Q = 0.707;
    *lowShelf .state = *FilterCoeff::makeLowShelf   ((double)sampleRate, (double)lowHz,  Q, juce::Decibels::decibelsToGain (lowDB));
    *midPeak  .state = *FilterCoeff::makePeakFilter ((double)sampleRate, (double)midHz,  Q, juce::Decibels::decibelsToGain (midDB));
    *highShelf.state = *FilterCoeff::makeHighShelf  ((double)sampleRate, (double)highHz, Q, juce::Decibels::decibelsToGain (highDB));
    lastLowDB = lowDB;  lastLowHz  = lowHz;
    lastMidDB = midDB;  lastMidHz  = midHz;
    lastHighDB= highDB; lastHighHz = highHz;
}

void AudioEffectChain::setEQ (float lowDB,  float lowHz,
                               float midDB,  float midHz,
                               float highDB, float highHz)
{
    if (lowDB != lastLowDB || lowHz != lastLowHz ||
        midDB != lastMidDB || midHz != lastMidHz ||
        highDB!= lastHighDB|| highHz!= lastHighHz)
        applyEQCoeffs (lowDB, lowHz, midDB, midHz, highDB, highHz);
}

void AudioEffectChain::setReverb (float roomSize, float wet)
{
    juce::dsp::Reverb::Parameters rp;
    rp.roomSize = juce::jlimit (0.0f, 1.0f, roomSize);
    rp.wetLevel = juce::jlimit (0.0f, 1.0f, wet);
    rp.dryLevel = 1.0f - rp.wetLevel * 0.5f;
    rp.damping  = 0.5f;
    rp.width    = 1.0f;
    reverbWet   = wet;
    reverb.setParameters (rp);
}

void AudioEffectChain::setDelay (float timeMs, float feedback, float wet)
{
    delayLenSamples = juce::jlimit (1, MAX_DELAY_SAMPLES - 1,
                                    (int)(timeMs * 0.001f * (float)sampleRate));
    delayFeedback   = juce::jlimit (0.0f, 0.92f, feedback);
    delayWet        = juce::jlimit (0.0f, 1.0f,  wet);
}

//==============================================================================
void AudioEffectChain::processBlock (juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float>          block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    lowShelf .process (ctx);
    midPeak  .process (ctx);
    highShelf.process (ctx);

    // Reverb (JUCE's reverb operates on stereo in-place)
    if (reverbWet > 0.001f)
        reverb.process (ctx);

    // Delay
    if (delayWet > 0.001f)
    {
        const int numSamples = buffer.getNumSamples();
        float* L = buffer.getWritePointer (0);
        float* R = numChannels > 1 ? buffer.getWritePointer (1) : L;

        for (int i = 0; i < numSamples; ++i)
        {
            int readPos = (delayWrite - delayLenSamples + MAX_DELAY_SAMPLES) % MAX_DELAY_SAMPLES;

            float dL = delayBufL[readPos];
            float dR = delayBufR[readPos];

            delayBufL[delayWrite] = L[i] + dL * delayFeedback;
            delayBufR[delayWrite] = R[i] + dR * delayFeedback;

            L[i] += dL * delayWet;
            R[i] += dR * delayWet;

            delayWrite = (delayWrite + 1) % MAX_DELAY_SAMPLES;
        }
    }
}
