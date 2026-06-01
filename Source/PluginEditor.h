#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class ChordDetectEditor : public juce::AudioProcessorEditor,
                           private juce::Timer
{
public:
    explicit ChordDetectEditor (ChordDetectProcessor&);
    ~ChordDetectEditor() override;

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    void timerCallback() override;

    void drawBackground    (juce::Graphics& g);
    void drawSkullMic      (juce::Graphics& g, juce::Rectangle<float> area, bool active);
    void drawChordPanel    (juce::Graphics& g, juce::Rectangle<float> area);
    void drawPianoKeyboard (juce::Graphics& g, juce::Rectangle<float> area,
                            const std::set<int>& active);

    ChordDetectProcessor& processor;

    ChordDetector::ChordInfo lastChord;
    std::set<int>            lastNotes;

    // Colour palette
    static constexpr juce::uint32 COL_BG    = 0xFF060A06;  // deep black-green
    static constexpr juce::uint32 COL_PANEL = 0xFF0C180D;  // dark panel
    static constexpr juce::uint32 COL_GREEN = 0xFF00FF6A;  // neon green — active
    static constexpr juce::uint32 COL_RED   = 0xFFFF1744;  // hot red — inactive
    static constexpr juce::uint32 COL_DIM   = 0xFF3A7D44;  // muted green
    static constexpr juce::uint32 COL_WHITE = 0xFFEEF5EE;  // off-white piano keys

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordDetectEditor)
};
