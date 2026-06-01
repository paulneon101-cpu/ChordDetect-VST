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
    void drawControlsStrip (juce::Graphics& g, juce::Rectangle<float> area);
    void drawPianoKeyboard (juce::Graphics& g, juce::Rectangle<float> area,
                            const std::set<int>& active);

    ChordDetectProcessor& processor;

    ChordDetector::ChordInfo lastChord;
    std::set<int>            lastNotes;

    // ── Controls ──────────────────────────────────────────────────────────
    juce::Slider    pitchSlider;
    juce::TextButton latBtn { "LOW" }, balBtn { "BAL" }, highBtn { "HIGH" };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> pitchAttachment;

    // Colour palette
    static constexpr juce::uint32 COL_BG    = 0xFF060A06;
    static constexpr juce::uint32 COL_PANEL = 0xFF0C180D;
    static constexpr juce::uint32 COL_GREEN = 0xFF00FF6A;
    static constexpr juce::uint32 COL_RED   = 0xFFFF1744;
    static constexpr juce::uint32 COL_DIM   = 0xFF3A7D44;
    static constexpr juce::uint32 COL_WHITE = 0xFFEEF5EE;

    void styleLatencyButton (juce::TextButton& btn, bool selected);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordDetectEditor)
};
