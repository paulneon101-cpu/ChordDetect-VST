#include "ChordAdvisor.h"
#include <JuceHeader.h>
#include <algorithm>

static const char* const NOTE_NAMES[12] = {
    "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
};

// Intervals for common qualities (semitones from root)
static const std::vector<int> INTERVALS_MAJ    = {0, 4, 7};
static const std::vector<int> INTERVALS_MIN    = {0, 3, 7};
static const std::vector<int> INTERVALS_DOM7   = {0, 4, 7, 10};
static const std::vector<int> INTERVALS_MAJ7   = {0, 4, 7, 11};
static const std::vector<int> INTERVALS_MIN7   = {0, 3, 7, 10};
static const std::vector<int> INTERVALS_DIM    = {0, 3, 6};
static const std::vector<int> INTERVALS_SUS4   = {0, 5, 7};

//==============================================================================
ChordAdvisor::Suggestion ChordAdvisor::makeSuggestion (int rootPc,
                                                        const std::string& quality,
                                                        float weight) const
{
    Suggestion s;
    s.rootNote  = rootPc;
    s.quality   = quality;
    s.name      = std::string (NOTE_NAMES[rootPc]) + (quality == "maj" ? "" : quality);
    s.weight    = weight;
    s.midiNotes = chordNotes (rootPc, quality);
    return s;
}

std::vector<int> ChordAdvisor::chordNotes (int rootPc, const std::string& quality) const
{
    const std::vector<int>* ivs = &INTERVALS_MAJ;
    if      (quality == "min")  ivs = &INTERVALS_MIN;
    else if (quality == "7")    ivs = &INTERVALS_DOM7;
    else if (quality == "maj7") ivs = &INTERVALS_MAJ7;
    else if (quality == "min7") ivs = &INTERVALS_MIN7;
    else if (quality == "dim")  ivs = &INTERVALS_DIM;
    else if (quality == "sus4") ivs = &INTERVALS_SUS4;

    // Build notes in octave 4 (base = 60 = C4)
    const int base = 60 + ((rootPc - 0 + 12) % 12);
    std::vector<int> notes;
    for (int iv : *ivs)
        notes.push_back (juce::jlimit (0, 127, base + iv));
    return notes;
}

//==============================================================================
// Common major-key progressions from each scale degree
std::vector<ChordAdvisor::Suggestion>
ChordAdvisor::majorProgressions (int rootPc) const
{
    // Degrees relative to root (semitones): 0=I 2=II 4=III 5=IV 7=V 9=VI 11=VII
    const int ii  = (rootPc + 2)  % 12;
    const int iii = (rootPc + 4)  % 12;
    const int IV  = (rootPc + 5)  % 12;
    const int V   = (rootPc + 7)  % 12;
    const int vi  = (rootPc + 9)  % 12;
    const int vii = (rootPc + 11) % 12;

    return {
        makeSuggestion (IV,  "maj",  0.90f),   // I → IV  (very common)
        makeSuggestion (V,   "7",    0.88f),   // I → V7
        makeSuggestion (vi,  "min",  0.80f),   // I → vi
        makeSuggestion (ii,  "min7", 0.70f),   // I → ii7
        makeSuggestion (iii, "min",  0.50f),   // I → iii
        makeSuggestion (vii, "dim",  0.30f),   // I → vii°
    };
}

std::vector<ChordAdvisor::Suggestion>
ChordAdvisor::minorProgressions (int rootPc) const
{
    const int ii  = (rootPc + 2)  % 12;   // ii°
    const int III = (rootPc + 3)  % 12;   // bIII
    const int iv  = (rootPc + 5)  % 12;   // iv
    const int V   = (rootPc + 7)  % 12;   // V (dominant)
    const int VI  = (rootPc + 8)  % 12;   // bVI
    const int VII = (rootPc + 10) % 12;   // bVII

    return {
        makeSuggestion (iv,  "min",  0.92f),   // i → iv
        makeSuggestion (V,   "7",    0.88f),   // i → V7
        makeSuggestion (VI,  "maj",  0.82f),   // i → bVI
        makeSuggestion (III, "maj",  0.75f),   // i → bIII
        makeSuggestion (VII, "maj",  0.65f),   // i → bVII
        makeSuggestion (ii,  "dim",  0.40f),   // i → ii°
    };
}

//==============================================================================
std::vector<ChordAdvisor::Suggestion>
ChordAdvisor::suggest (const ChordDetector::ChordInfo& current) const
{
    if (!current.isValid)
    {
        // No chord detected — offer common starting chords
        return {
            makeSuggestion (0,  "maj",  1.0f),
            makeSuggestion (9,  "min",  0.9f),
            makeSuggestion (5,  "maj",  0.8f),
            makeSuggestion (7,  "maj",  0.7f),
            makeSuggestion (2,  "min",  0.6f),
        };
    }

    // Select progressions based on chord quality
    const bool isMinor = (current.quality == "min"  || current.quality == "min7" ||
                          current.quality == "dim"   || current.quality == "dim7" ||
                          current.quality == "m7b5");

    auto suggestions = isMinor
        ? minorProgressions (current.rootNote)
        : majorProgressions (current.rootNote);

    // Sort by weight descending, return top 5
    std::sort (suggestions.begin(), suggestions.end(),
        [](const Suggestion& a, const Suggestion& b){ return a.weight > b.weight; });

    if (suggestions.size() > 5) suggestions.resize (5);
    return suggestions;
}
