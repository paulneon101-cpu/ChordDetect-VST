#include "HarmonyGenerator.h"

void HarmonyGenerator::setMode (Mode m)
{
    mode  = m;
    dirty = true;
}

//==============================================================================
bool HarmonyGenerator::update (const ChordDetector::ChordInfo& chord,
                                const std::set<int>&            pitchClasses,
                                int                             voiceMidiNote)
{
    if (mode == Mode::Off)
    {
        if (!activeNotes.empty()) { pendingNotes.clear(); dirty = true; }
        return dirty;
    }

    bool changed = (chord.name    != lastChord.name)
                || (chord.isValid != lastChord.isValid)
                || (voiceMidiNote != lastVoiceNote);

    if (changed || dirty)
    {
        lastChord     = chord;
        lastVoiceNote = voiceMidiNote;
        pendingNotes  = buildNotes (chord, pitchClasses, voiceMidiNote);
        dirty         = (pendingNotes != activeNotes);
    }

    return dirty;
}

void HarmonyGenerator::fillMidi (juce::MidiBuffer& dest, int offset, int ch)
{
    if (!dirty) return;

    // Note-offs for removed notes
    for (int n : activeNotes)
        if (pendingNotes.find (n) == pendingNotes.end())
            dest.addEvent (juce::MidiMessage::noteOff  (ch, n, (uint8_t)0),   offset);

    // Note-ons for new notes
    for (int n : pendingNotes)
        if (activeNotes.find (n) == activeNotes.end())
            dest.addEvent (juce::MidiMessage::noteOn   (ch, n, (uint8_t)100), offset);

    activeNotes = pendingNotes;
    dirty       = false;
}

void HarmonyGenerator::allNotesOff (juce::MidiBuffer& dest, int offset, int ch)
{
    for (int n : activeNotes)
        dest.addEvent (juce::MidiMessage::noteOff (ch, n, (uint8_t)0), offset);
    activeNotes .clear();
    pendingNotes.clear();
    dirty = false;
}

//==============================================================================
std::set<int> HarmonyGenerator::buildNotes (const ChordDetector::ChordInfo& chord,
                                              const std::set<int>&            pitchClasses,
                                              int                             voiceMidiNote) const
{
    std::set<int> result;

    if (mode == Mode::FullChord)
    {
        // Output the whole chord across one octave centred on middle C (C4 = 60)
        if (!chord.isValid || pitchClasses.empty()) return result;
        int base = 60;
        for (int pc : pitchClasses)
        {
            int note = base + ((pc - (base % 12) + 12) % 12);
            if (note > 84) note -= 12;
            result.insert (juce::jlimit (0, 127, note));
        }
        return result;
    }

    // For interval modes we need a source note to harmonise
    int srcNote = voiceMidiNote;
    if (srcNote < 0)
    {
        // Fall back to chord root in octave 4
        if (!chord.isValid) return result;
        srcNote = 60 + ((chord.rootNote - 60 % 12 + 12) % 12);
    }

    int harmNote = srcNote;
    switch (mode)
    {
        case Mode::Third:
        {
            // Use chord quality to decide major (4 st) or minor (3 st) 3rd
            int interval = (chord.quality.find ("min") != std::string::npos ||
                            chord.quality == "dim" || chord.quality == "dim7") ? 3 : 4;
            harmNote = srcNote + interval;
            break;
        }
        case Mode::Fifth:
            harmNote = srcNote + 7;
            break;
        case Mode::Octave:
            harmNote = srcNote + 12;
            break;
        default: break;
    }

    harmNote = juce::jlimit (0, 127, harmNote);
    if (harmNote != srcNote)
        result.insert (harmNote);

    return result;
}
