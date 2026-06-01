#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class ChordDetectEditor : public juce::AudioProcessorEditor,
                           private juce::Timer
{
public:
    explicit ChordDetectEditor (ChordDetectProcessor&);
    ~ChordDetectEditor() override;

    void paint     (juce::Graphics&) override;
    void resized   () override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    void timerCallback() override;

    // ── Paint helpers ────────────────────────────────────────────────
    void paintBackground   (juce::Graphics&);
    void paintTitleBar     (juce::Graphics&, juce::Rectangle<int>);
    void paintChordBody    (juce::Graphics&, juce::Rectangle<int>);
    void paintSkullMic     (juce::Graphics&, juce::Rectangle<float>, bool active);
    void paintChordDisplay (juce::Graphics&, juce::Rectangle<float>);
    void paintPianoRoll    (juce::Graphics&, juce::Rectangle<int>);
    void paintPianoKeys    (juce::Graphics&, juce::Rectangle<int>);
    void paintSectionLabel (juce::Graphics&, juce::Rectangle<int>,
                            const juce::String&, juce::uint32 col);
    void paintCowboyHat    (juce::Graphics&, float cx, float topY,
                            float w, juce::Colour);
    void paintJohnnyNixLogo(juce::Graphics&, juce::Rectangle<float>, juce::Colour);
    void paintEQLabels     (juce::Graphics&);
    void paintFXLabels     (juce::Graphics&);

    // ── Layout constants (set in resized) ────────────────────────────
    struct Layout {
        juce::Rectangle<int> title, body;
        juce::Rectangle<int> strip1, strip2, strip3;   // pitch/lat, harmony, voice
        juce::Rectangle<int> eqStrip, fxStrip;
        juce::Rectangle<int> aiStrip;
        juce::Rectangle<int> midiCtrl, pianoRoll;
        juce::Rectangle<int> pianoKeys;
    } lay;

    ChordDetectProcessor& proc;
    ChordDetector::ChordInfo             lastChord;
    std::set<int>                        lastNotes;
    int                                  lastVoiceNote { -1 };
    std::vector<ChordAdvisor::Suggestion> lastSuggestions;

    // ── Sliders ──────────────────────────────────────────────────────
    juce::Slider pitchSlider, voiceSensSlider;
    juce::Slider eqLowSlider,   eqLowHzSlider;
    juce::Slider eqMidSlider,   eqMidHzSlider;
    juce::Slider eqHighSlider,  eqHighHzSlider;
    juce::Slider revRoomSlider, revWetSlider;
    juce::Slider dlyTimeSlider, dlyFdbkSlider, dlyWetSlider;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SA> pitchAtt, voiceSensAtt;
    std::unique_ptr<SA> eqLowAtt,   eqLowHzAtt;
    std::unique_ptr<SA> eqMidAtt,   eqMidHzAtt;
    std::unique_ptr<SA> eqHighAtt,  eqHighHzAtt;
    std::unique_ptr<SA> revRoomAtt, revWetAtt;
    std::unique_ptr<SA> dlyTimeAtt, dlyFdbkAtt, dlyWetAtt;

    // ── Buttons ──────────────────────────────────────────────────────
    juce::TextButton latLowBtn{"LOW"}, latBalBtn{"BAL"}, latHiBtn{"HI"};

    juce::TextButton harmOffBtn{"OFF"}, harm3Btn{"+3RD"},
                     harm5Btn{"+5TH"},  harmOctBtn{"+OCT"}, harmChordBtn{"CHORD"};

    juce::TextButton voiceOnBtn{"MIC > MIDI"};

    juce::TextButton midiRecBtn{"REC"}, midiClrBtn{"CLR"},
                     midiQntBtn{"QNT"},   midiM12Btn{"-12"},
                     midiM1Btn{"-1"},     midiP1Btn{"+1"},  midiP12Btn{"+12"};

    // AI suggestion buttons (up to 5)
    juce::TextButton suggBtn[5];

    // ── Colours ──────────────────────────────────────────────────────
    static constexpr juce::uint32 C_BG    = 0xFF080D08;   // deep black-green
    static constexpr juce::uint32 C_PANEL = 0xFF101E11;   // raised panel
    static constexpr juce::uint32 C_GREEN = 0xFF39FF84;   // neon mint — very bright
    static constexpr juce::uint32 C_RED   = 0xFFFF3355;   // hot red
    static constexpr juce::uint32 C_AMBER = 0xFFFFCC00;   // golden amber
    static constexpr juce::uint32 C_CYAN  = 0xFF18FFFF;   // electric cyan
    static constexpr juce::uint32 C_DIM   = 0xFF52A863;   // medium green
    static constexpr juce::uint32 C_WHITE = 0xFFF0FAF0;   // cool white

    // ── Helpers ──────────────────────────────────────────────────────
    void styleBtn (juce::TextButton&, bool on, juce::uint32 onCol = C_GREEN);
    void addSlider (juce::Slider&, juce::Slider::SliderStyle, juce::uint32 col);

    void setupLatencyBtns();
    void setupHarmonyBtns();
    void setupSuggestionBtns();

    // ── Virtual keyboard state ────────────────────────────────────────
    int  keyboardBaseNote { 48 };     // C3 — shift with OCT- / OCT+ buttons
    int  heldVirtualNote  { -1 };     // note currently pressed by mouse

    int  pianoNoteAtPoint (juce::Point<int> pos) const;
    void virtualNoteOn    (int midiNote);
    void virtualNoteOff   (int midiNote);

    juce::TextButton octDownBtn { "OCT -" }, octUpBtn { "OCT +" };

    static const int WHITE_KEYS[7];
    static const int BLACK_KEYS[7];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordDetectEditor)
};
