#include "ChordDetector.h"
#include <limits>
#include <algorithm>

static const char* const NOTE_NAMES[12] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

ChordDetector::ChordDetector()
{
    buildTemplates();
}

void ChordDetector::buildTemplates()
{
    // Ordered roughly by prevalence; earlier entries win ties
    templates = {
        // Triads
        { "maj",       { 0, 4, 7 }           },
        { "min",       { 0, 3, 7 }           },
        { "dim",       { 0, 3, 6 }           },
        { "aug",       { 0, 4, 8 }           },
        { "sus2",      { 0, 2, 7 }           },
        { "sus4",      { 0, 5, 7 }           },
        { "5",         { 0, 7 }              },
        // Sevenths
        { "maj7",      { 0, 4, 7, 11 }       },
        { "7",         { 0, 4, 7, 10 }       },
        { "min7",      { 0, 3, 7, 10 }       },
        { "minmaj7",   { 0, 3, 7, 11 }       },
        { "dim7",      { 0, 3, 6, 9 }        },
        { "m7b5",      { 0, 3, 6, 10 }       },
        { "aug7",      { 0, 4, 8, 10 }       },
        // Sixths
        { "6",         { 0, 4, 7, 9 }        },
        { "min6",      { 0, 3, 7, 9 }        },
        // Ninths
        { "maj9",      { 0, 4, 7, 11, 14 }   },
        { "9",         { 0, 4, 7, 10, 14 }   },
        { "min9",      { 0, 3, 7, 10, 14 }   },
        { "add9",      { 0, 4, 7, 14 }       },
        { "minadd9",   { 0, 3, 7, 14 }       },
        // Elevenths / thirteenths
        { "11",        { 0, 4, 7, 10, 14, 17 }          },
        { "maj11",     { 0, 4, 7, 11, 14, 17 }          },
        { "13",        { 0, 4, 7, 10, 14, 17, 21 }      },
        { "maj13",     { 0, 4, 7, 11, 14, 17, 21 }      },
    };
}

int ChordDetector::score(const std::set<int>& pcs,
                          const std::vector<int>& intervals,
                          int root) const
{
    int hits = 0, misses = 0;

    for (int iv : intervals)
    {
        if (pcs.count((root + iv) % 12))
            ++hits;
        else
            ++misses;
    }

    // Penalise input notes not covered by the template
    for (int pc : pcs)
    {
        bool covered = false;
        for (int iv : intervals)
            if ((root + iv) % 12 == pc) { covered = true; break; }
        if (!covered) ++misses;
    }

    // Reward completeness: full template match scores higher than partial
    const int templateSize = (int)intervals.size();
    return hits * 10 - misses * 4 + (hits == templateSize ? 5 : 0);
}

ChordDetector::ChordInfo ChordDetector::detect(const std::set<int>& pcs) const
{
    if (pcs.size() < 2)
        return {};

    ChordInfo best;
    int       bestScore = std::numeric_limits<int>::min();

    for (int root = 0; root < 12; ++root)
    {
        for (const auto& tmpl : templates)
        {
            int s = score(pcs, tmpl.intervals, root);
            if (s > bestScore)
            {
                bestScore       = s;
                best.rootNote   = root;
                best.root       = NOTE_NAMES[root];
                best.quality    = tmpl.quality;
                best.name       = best.root + (tmpl.quality == "maj" ? "" : tmpl.quality);
                best.isValid    = true;
            }
        }
    }

    if (bestScore < 10)
        best.isValid = false;

    return best;
}
