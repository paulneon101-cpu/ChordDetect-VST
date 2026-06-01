#pragma once
#include <string>
#include <vector>
#include <set>

class ChordDetector
{
public:
    struct ChordInfo
    {
        std::string name;
        std::string root;
        std::string quality;
        int         rootNote { -1 };
        bool        isValid  { false };
    };

    ChordDetector();

    ChordInfo detect(const std::set<int>& pitchClasses) const;

private:
    struct ChordTemplate
    {
        std::string      quality;
        std::vector<int> intervals;
    };

    std::vector<ChordTemplate> templates;

    void buildTemplates();
    int  score(const std::set<int>& pcs, const std::vector<int>& intervals, int root) const;
};
