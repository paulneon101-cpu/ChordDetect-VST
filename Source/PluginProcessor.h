#pragma once
#include <JuceHeader.h>
#include "PitchDetector.h"
#include "MonoPitchDetector.h"
#include "ChordDetector.h"
#include "HarmonyGenerator.h"
#include "AudioEffectChain.h"
#include "MidiRecorder.h"
#include "ChordAdvisor.h"

class ChordDetectProcessor : public juce::AudioProcessor
{
public:
    ChordDetectProcessor();
    ~ChordDetectProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor()  const override { return true; }

    const juce::String getName()          const override { return "ChordDetect"; }
    bool   acceptsMidi()  const override { return true;  }
    bool   producesMidi() const override { return true;  }
    bool   isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int  getNumPrograms()    override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    //==========================================================================
    ChordDetector::ChordInfo              getCurrentChord()      const;
    std::set<int>                         getCurrentNotes()      const;
    int                                   getCurrentVoiceNote()  const;
    std::vector<MidiRecorder::NoteEvent>  getRecordedEvents()    const;
    double                                getRecorderTime()      const;
    std::vector<ChordAdvisor::Suggestion> getSuggestions()       const;

    // UI-driven actions (called from message thread)
    void clearRecording();
    void transposeRecording (int semitones);
    void quantizeRecording();
    void playSuggestion (int index, juce::MidiBuffer& dest);

    //==========================================================================
    static constexpr const char* PID_PITCH       = "pitchCorrection";
    static constexpr const char* PID_LATENCY     = "latencyMode";
    static constexpr const char* PID_HARMONY     = "harmonyMode";
    static constexpr const char* PID_VOICE_ON    = "voiceToMidi";
    static constexpr const char* PID_VOICE_SENS  = "voiceSensitivity";
    static constexpr const char* PID_EQ_LOW      = "eqLow";
    static constexpr const char* PID_EQ_LOW_HZ   = "eqLowHz";
    static constexpr const char* PID_EQ_MID      = "eqMid";
    static constexpr const char* PID_EQ_MID_HZ   = "eqMidHz";
    static constexpr const char* PID_EQ_HIGH     = "eqHigh";
    static constexpr const char* PID_EQ_HIGH_HZ  = "eqHighHz";
    static constexpr const char* PID_REV_ROOM    = "reverbRoom";
    static constexpr const char* PID_REV_WET     = "reverbWet";
    static constexpr const char* PID_DLY_TIME    = "delayTime";
    static constexpr const char* PID_DLY_FDBK    = "delayFeedback";
    static constexpr const char* PID_DLY_WET     = "delayWet";
    static constexpr const char* PID_MIDI_REC    = "midiRecord";

    juce::MidiKeyboardState            keyboardState;   // virtual + hardware keyboard
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    PitchDetector     pitchDetector;
    MonoPitchDetector monoPitchDetector;
    ChordDetector     chordDetector;
    HarmonyGenerator  harmonyGenerator;
    AudioEffectChain  fxChain;
    MidiRecorder      midiRecorder;
    ChordAdvisor      chordAdvisor;

    double currentSampleRate { 44100.0 };
    int    currentBlockSize  { 512 };
    int    lastLatencyMode   { 1 };
    int    lastVoiceMidiNote { -1 };

    static constexpr int VOICE_MIDI_CH   = 1;
    static constexpr int HARMONY_MIDI_CH = 2;

    mutable juce::CriticalSection dataLock;
    ChordDetector::ChordInfo              currentChord;
    std::set<int>                         currentNotes;
    int                                   currentVoiceNote { -1 };
    std::vector<ChordAdvisor::Suggestion> currentSuggestions;

    // Pending suggestion MIDI (played on next processBlock)
    juce::CriticalSection         suggLock;
    std::vector<int>              pendingSuggNotes;
    std::vector<int>              lastSuggNotes;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordDetectProcessor)
};
