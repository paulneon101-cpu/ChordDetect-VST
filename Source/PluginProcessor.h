#pragma once
#include <JuceHeader.h>
#include "PitchDetector.h"
#include "ChordDetector.h"

class ChordDetectProcessor : public juce::AudioProcessor
{
public:
    ChordDetectProcessor();
    ~ChordDetectProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return "ChordDetect"; }
    bool   acceptsMidi()  const override { return false; }
    bool   producesMidi() const override { return true;  }
    bool   isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==========================================================================
    int  getNumPrograms()   override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    //==========================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    ChordDetector::ChordInfo getCurrentChord() const;
    std::set<int>            getCurrentNotes() const;

private:
    PitchDetector pitchDetector;
    ChordDetector chordDetector;

    mutable juce::CriticalSection dataLock;
    ChordDetector::ChordInfo currentChord;
    std::set<int>            currentNotes;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordDetectProcessor)
};
