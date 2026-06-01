#pragma once
#include <JuceHeader.h>
#include <vector>

class AudioEffectChain
{
public:
    AudioEffectChain() = default;
    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void processBlock (juce::AudioBuffer<float>& buffer);

    void setEQ      (float lowDB,  float lowHz,
                     float midDB,  float midHz,
                     float highDB, float highHz);
    void setReverb  (float roomSize, float wet);
    void setDelay   (float timeMs,  float feedback, float wet);

private:
    double sampleRate   { 44100.0 };
    int    numChannels  { 2 };

    // ── 3-band EQ ──────────────────────────────────────────────────────
    using Filter    = juce::dsp::IIR::Filter<float>;
    using FilterCoeff = juce::dsp::IIR::Coefficients<float>;
    using DupFilter = juce::dsp::ProcessorDuplicator<Filter, FilterCoeff>;

    DupFilter lowShelf, midPeak, highShelf;
    float lastLowDB  { 999.f }, lastMidDB  { 999.f }, lastHighDB  { 999.f };
    float lastLowHz  { 0.f },  lastMidHz  { 0.f },  lastHighHz  { 0.f };
    void  applyEQCoeffs (float lowDB,  float lowHz,
                         float midDB,  float midHz,
                         float highDB, float highHz);

    // ── Reverb ─────────────────────────────────────────────────────────
    juce::dsp::Reverb reverb;
    float reverbWet { 0.0f };

    // ── Delay (stereo circular buffer) ─────────────────────────────────
    static constexpr int MAX_DELAY_SAMPLES = 96000;   // ~2 s @ 48 kHz
    std::vector<float> delayBufL, delayBufR;
    int   delayWrite       { 0 };
    int   delayLenSamples  { 22050 };
    float delayFeedback    { 0.3f };
    float delayWet         { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioEffectChain)
};
