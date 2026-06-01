#pragma once
#include <JuceHeader.h>
#include <vector>
#include <set>

class PitchDetector
{
public:
    PitchDetector();

    void prepare(double sampleRate, int blockSize);
    void processBlock(const juce::AudioBuffer<float>& buffer);
    std::set<int> getActiveNotes() const;

private:
    static constexpr int FFT_ORDER = 11;        // 2048-point FFT
    static constexpr int FFT_SIZE  = 1 << FFT_ORDER;

    juce::dsp::FFT                        fft;
    juce::dsp::WindowingFunction<float>   windowing;

    std::vector<float> fftData;
    std::vector<float> magnitudes;
    std::vector<float> fifo;
    int                fifoIndex { 0 };

    double sampleRate { 44100.0 };

    mutable juce::CriticalSection lock;
    std::set<int> activeNotes;

    void runFFT();
    int  frequencyToMidiNote(float freq) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchDetector)
};
