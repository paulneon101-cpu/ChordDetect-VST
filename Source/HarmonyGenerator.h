#pragma once
#include <JuceHeader.h>
#include "ChordDetector.h"
#include <set>
#include <vector>

class HarmonyGenerator
{
public:
    enum class Mode { Off = 0, Third, Fifth, Octave, FullChord };

    void setMode (Mode m);
    Mode getMode() const { return mode; }

    // Call each block with latest chord + optional voice note (-1 = none)
    // Returns true if MIDI output changed
    bool update (const ChordDetector::ChordInfo& chord,
                 const std::set<int>&            pitchClasses,
                 int                             voiceMidiNote);

    // Write note-off / note-on diff into dest at sampleOffset, midiChannel
    void fillMidi (juce::MidiBuffer& dest, int sampleOffset, int midiChannel);

    // Force all notes off (e.g. when bypassed)
    void allNotesOff (juce::MidiBuffer& dest, int sampleOffset, int midiChannel);

private:
    Mode mode { Mode::Off };

    std::set<int> activeNotes;   // currently sounding harmony MIDI notes

    ChordDetector::ChordInfo lastChord;
    int                      lastVoiceNote { -1 };
    bool                     dirty         { false };
    std::set<int>            pendingNotes;

    std::set<int> buildNotes (const ChordDetector::ChordInfo& chord,
                               const std::set<int>&            pitchClasses,
                               int                             voiceMidiNote) const;

    // Chromatic interval from root into chord's actual scale tone
    int chordInterval (int semitones, const std::set<int>& pitchClasses, int rootPc) const;
};
