#pragma once
#include "ChordDetector.h"
#include <string>
#include <vector>

class ChordAdvisor
{
public:
    struct Suggestion
    {
        std::string name;
        int         rootNote;       // 0-11 pitch class
        std::string quality;
        std::vector<int> midiNotes; // absolute MIDI notes (octave 4 base)
        float       weight;         // higher = more likely
    };

    // Returns up to 5 chord suggestions given the currently detected chord
    std::vector<Suggestion> suggest (const ChordDetector::ChordInfo& current) const;

private:
    std::vector<Suggestion> majorProgressions (int rootPc) const;
    std::vector<Suggestion> minorProgressions (int rootPc) const;

    Suggestion makeSuggestion (int rootPc, const std::string& quality, float weight) const;
    std::vector<int> chordNotes (int rootPc, const std::string& quality) const;
};
