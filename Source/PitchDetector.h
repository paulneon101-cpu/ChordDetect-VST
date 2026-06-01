#pragma once
#include <JuceHeader.h>
#include <vector>
#include <set>
#include <memory>

class PitchDetector
{
public:
    PitchDetector();

    void prepare (double sampleRate, int blockSize);
    void processBlock (const juce::AudioBuffer<float>& buffer);
    std::set<int> getActiveNotes() const;

    // 9 = 512-pt (~12 ms)  |  10 = 1024-pt (~23 ms)  |  11 = 2048-pt (~46 ms)
    void setFFTOrder (int order);

    // 0 = permissive (±50 ¢)  …  100 = strict (±5 ¢)
    void setPitchCorrection (float percent);

private:
    int   fftOrder               { 11 };
    float pitchCorrectionPercent { 50.0f };

    std::unique_ptr<juce::dsp::FFT>                      fft;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> windowing;

    std::vector<float> fftData;
    std::vector<float> magnitudes;
    std::vector<float> fifo;
    int                fifoIndex { 0 };

    double sampleRate { 44100.0 };

    mutable juce::CriticalSection lock;
    std::set<int> activeNotes;

    void initFFT();
    void runFFT();
    int  frequencyToMidiNote (float freq) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchDetector)
};
