#pragma once
#include <JuceHeader.h>
#include <vector>

// YIN-based monophonic pitch detector — mic input -> MIDI note
class MonoPitchDetector
{
public:
    MonoPitchDetector();

    void prepare (double sampleRate, int blockSize);
    void processBlock (const juce::AudioBuffer<float>& buffer);

    void setEnabled     (bool e);
    void setSensitivity (float s);   // 0 = loose, 1 = strict

    int   getCurrentMidiNote() const;   // -1 if no pitch detected
    float getConfidence()       const;
    bool  isEnabled()           const;

private:
    static constexpr int FRAME_SIZE = 2048;

    bool   enabled     { false };
    float  sensitivity { 0.5f };
    double sampleRate  { 44100.0 };

    std::vector<float> fifo;
    int                fifoIndex { 0 };
    std::vector<float> yinBuf;

    mutable juce::CriticalSection lock;
    int   currentNote       { -1 };
    float currentConfidence { 0.0f };

    float detectPitch (const float* samples, int numSamples);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MonoPitchDetector)
};
