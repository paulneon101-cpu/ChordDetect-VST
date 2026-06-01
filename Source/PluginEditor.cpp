#include "PluginEditor.h"

static const char* const NOTE_NAMES[12] = {
    "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
};

static const int WHITE_KEYS[7] = { 0, 2, 4, 5, 7, 9, 11 };
static const int BLACK_KEYS[7] = { 1, 3, -1, 6, 8, 10, -1 };

//==============================================================================
ChordDetectEditor::ChordDetectEditor (ChordDetectProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (520, 380);
    setResizable (true, true);
    setResizeLimits (420, 320, 960, 700);

    // ── Pitch Correction slider ─────────────────────────────────────────
    pitchSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    pitchSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 46, 20);
    pitchSlider.setColour (juce::Slider::thumbColourId,           juce::Colour (COL_GREEN));
    pitchSlider.setColour (juce::Slider::trackColourId,           juce::Colour (COL_GREEN).withAlpha (0.45f));
    pitchSlider.setColour (juce::Slider::backgroundColourId,      juce::Colour (COL_PANEL));
    pitchSlider.setColour (juce::Slider::textBoxTextColourId,     juce::Colour (COL_GREEN));
    pitchSlider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (COL_BG));
    pitchSlider.setColour (juce::Slider::textBoxOutlineColourId,  juce::Colours::transparentBlack);
    addAndMakeVisible (pitchSlider);

    pitchAttachment = std::make_unique<SliderAttachment> (
        processor.apvts, ChordDetectProcessor::PID_PITCH, pitchSlider);

    // ── Latency buttons ─────────────────────────────────────────────────
    auto* latParam = processor.apvts.getRawParameterValue (ChordDetectProcessor::PID_LATENCY);
    int   curMode  = (int)latParam->load();

    for (auto* [btn, idx] : std::initializer_list<std::pair<juce::TextButton*, int>> {
            { &latBtn, 0 }, { &balBtn, 1 }, { &highBtn, 2 } })
    {
        styleLatencyButton (*btn, idx == curMode);
        btn->onClick = [this, btn, idx]() mutable
        {
            auto* param = dynamic_cast<juce::AudioParameterChoice*> (
                processor.apvts.getParameter (ChordDetectProcessor::PID_LATENCY));
            if (param) param->setValueNotifyingHost (param->convertTo0to1 ((float)idx));
            styleLatencyButton (latBtn,  idx == 0);
            styleLatencyButton (balBtn,  idx == 1);
            styleLatencyButton (highBtn, idx == 2);
        };
        addAndMakeVisible (*btn);
    }

    startTimerHz (20);
}

ChordDetectEditor::~ChordDetectEditor()
{
    stopTimer();
}

//==============================================================================
void ChordDetectEditor::styleLatencyButton (juce::TextButton& btn, bool selected)
{
    btn.setColour (juce::TextButton::buttonColourId,
                   selected ? juce::Colour (COL_GREEN).withAlpha (0.25f)
                            : juce::Colour (COL_PANEL));
    btn.setColour (juce::TextButton::textColourOffId,
                   selected ? juce::Colour (COL_GREEN) : juce::Colour (COL_DIM));
    btn.setColour (juce::ComboBox::outlineColourId,
                   selected ? juce::Colour (COL_GREEN).withAlpha (0.6f)
                            : juce::Colour (COL_DIM).withAlpha (0.3f));
}

//==============================================================================
void ChordDetectEditor::paint (juce::Graphics& g)
{
    const bool active = lastChord.isValid && !lastNotes.empty();
    auto       bounds = getLocalBounds().toFloat();

    drawBackground (g);

    // Title bar
    auto titleBar = bounds.removeFromTop (46.0f);
    auto iconArea = titleBar.removeFromLeft (46.0f).reduced (6.0f);
    drawSkullMic (g, iconArea, active);

    g.setColour (active ? juce::Colour (COL_GREEN) : juce::Colour (COL_RED));
    g.setFont   (juce::Font ("Helvetica Neue", 17.0f, juce::Font::bold));
    g.drawText  ("CHORDDETECT  VST3", titleBar, juce::Justification::centredLeft);

    g.setColour (juce::Colour (active ? COL_GREEN : COL_RED).withAlpha (0.25f));
    g.fillRect  (0.0f, 46.0f, (float)getWidth(), 1.0f);

    // Body (skull + chord), above controls strip + piano
    const float pianoH    = 100.0f;
    const float controlsH = 52.0f;
    auto body = bounds.withTrimmedBottom (pianoH + controlsH);

    auto skullArea = body.removeFromLeft (body.getWidth() * 0.38f).reduced (10.0f, 6.0f);
    drawSkullMic   (g, skullArea, active);
    drawChordPanel (g, body.reduced (8.0f, 6.0f));

    // Controls strip label (components draw themselves, we just paint labels)
    auto ctrlArea = getLocalBounds().toFloat()
                        .withTrimmedTop    (46.0f + (getHeight() - 46.0f - pianoH - controlsH))
                        .withTrimmedBottom (pianoH);
    drawControlsStrip (g, ctrlArea);

    // Piano
    auto pianoArea = getLocalBounds().toFloat().removeFromBottom (pianoH).reduced (14.0f, 10.0f);
    drawPianoKeyboard (g, pianoArea, lastNotes);
}

void ChordDetectEditor::resized()
{
    const int pianoH    = 100;
    const int controlsH = 52;
    const int titleH    = 46;

    // Controls strip sits between body and piano
    auto controlsRect = getLocalBounds()
                            .withTrimmedTop    (getHeight() - pianoH - controlsH)
                            .withTrimmedBottom (pianoH)
                            .reduced (14, 8);

    // Left half: pitch slider with label space
    auto pitchArea = controlsRect.removeFromLeft (controlsRect.getWidth() / 2);
    pitchSlider.setBounds (pitchArea.withTrimmedLeft (90));  // room for "PITCH CORRECT" label

    // Right half: three latency buttons
    auto latArea = controlsRect.reduced (4, 0);
    int  btnW    = latArea.getWidth() / 3;
    latBtn  .setBounds (latArea.removeFromLeft (btnW).reduced (2, 0));
    balBtn  .setBounds (latArea.removeFromLeft (btnW).reduced (2, 0));
    highBtn .setBounds (latArea.reduced (2, 0));

    juce::ignoreUnused (titleH);
}

//==============================================================================
void ChordDetectEditor::timerCallback()
{
    auto chord = processor.getCurrentChord();
    auto notes = processor.getCurrentNotes();
    if (chord.name != lastChord.name || notes != lastNotes)
    {
        lastChord = chord;
        lastNotes = notes;
        repaint();
    }
}

//==============================================================================
void ChordDetectEditor::drawBackground (juce::Graphics& g)
{
    const bool active = lastChord.isValid && !lastNotes.empty();
    auto b = getLocalBounds().toFloat();

    g.setColour (juce::Colour (COL_BG));
    g.fillRect  (b);

    juce::Colour glowTop = active ? juce::Colour (COL_GREEN).withAlpha (0.10f)
                                  : juce::Colour (COL_RED)  .withAlpha (0.08f);
    juce::ColourGradient vignette (glowTop, b.getCentreX(), 0.0f,
                                    juce::Colour (COL_BG).withAlpha (0.0f),
                                    b.getCentreX(), b.getHeight() * 0.6f, true);
    g.setGradientFill (vignette);
    g.fillRect (b);

    g.setColour (juce::Colours::black.withAlpha (0.06f));
    for (float y = 0.0f; y < b.getHeight(); y += 3.0f)
        g.fillRect (0.0f, y, b.getWidth(), 1.0f);
}

//==============================================================================
void ChordDetectEditor::drawControlsStrip (juce::Graphics& g, juce::Rectangle<float> area)
{
    // Panel bg
    g.setColour (juce::Colour (COL_PANEL).withAlpha (0.8f));
    g.fillRoundedRectangle (area.reduced (2.0f), 6.0f);
    g.setColour (juce::Colour (COL_DIM).withAlpha (0.3f));
    g.drawRoundedRectangle (area.reduced (2.0f), 6.0f, 1.0f);

    // "PITCH CORRECT" label
    auto pitchLabelArea = area.withWidth (area.getWidth() / 2.0f)
                              .withTrimmedLeft (10.0f);
    g.setColour (juce::Colour (COL_DIM));
    g.setFont   (juce::Font ("Helvetica Neue", 10.5f, juce::Font::bold));
    g.drawText  ("PITCH CORRECT", pitchLabelArea, juce::Justification::centredLeft);

    // "LATENCY" label above buttons
    auto latLabelArea = area.withTrimmedLeft (area.getWidth() / 2.0f)
                            .withTrimmedRight (6.0f)
                            .withHeight (16.0f);
    g.drawText ("LATENCY", latLabelArea, juce::Justification::centred);
}

//==============================================================================
void ChordDetectEditor::drawSkullMic (juce::Graphics& g,
                                       juce::Rectangle<float> area,
                                       bool active)
{
    const float cx = area.getCentreX();
    const float cy = area.getCentreY();
    const float h  = area.getHeight();
    const float s  = h / 100.0f;

    const juce::Colour col  = active ? juce::Colour (COL_GREEN) : juce::Colour (COL_RED);
    const juce::Colour glow = col.withAlpha (0.18f);
    const juce::Colour bg   = juce::Colour (COL_BG);

    // Mic handle
    const float hndX = cx - 8.0f * s, hndY = cy + 16.0f * s;
    const float hndW = 16.0f * s,     hndH = 24.0f * s;

    g.setColour (col.withAlpha (0.75f));
    g.fillRoundedRectangle (hndX, hndY, hndW, hndH, 7.0f * s);

    g.setColour (bg.withAlpha (0.45f));
    for (int i = 1; i <= 5; ++i)
        g.fillRect (hndX + 2.0f*s, hndY + i * (hndH / 6.0f), hndW - 4.0f*s, 1.0f);

    g.setColour (col.withAlpha (0.55f));
    g.fillRoundedRectangle (cx - 14.0f*s, hndY + hndH + 1.0f*s, 28.0f*s, 5.0f*s, 2.5f*s);

    // Skull head
    const float headTop = cy - 40.0f * s;
    const float headW   = 38.0f * s;
    const float headH   = 36.0f * s;

    juce::Path halo;
    halo.addEllipse (cx - headW * 0.65f, headTop - headH * 0.1f, headW * 1.3f, headH * 1.2f);
    g.setColour (glow);
    g.fillPath (halo);

    juce::Path cranium;
    cranium.addEllipse (cx - headW * 0.5f, headTop, headW, headH);
    g.setColour (bg);
    g.fillPath (cranium);
    g.setColour (col);
    g.strokePath (cranium, juce::PathStrokeType (1.8f));

    const float jawTop = headTop + headH * 0.62f;
    const float jawW   = headW   * 0.76f;
    const float jawH   = headH   * 0.46f;
    juce::Path jaw;
    jaw.addRoundedRectangle (cx - jawW * 0.5f, jawTop, jawW, jawH, jawW * 0.18f);
    g.setColour (bg);
    g.fillPath (jaw);
    g.setColour (col);
    g.strokePath (jaw, juce::PathStrokeType (1.5f));

    // Eyes
    const float eyeW = headW * 0.22f, eyeH = headH * 0.28f;
    const float eyeY = headTop + headH * 0.18f;
    for (int side : { -1, 1 })
    {
        float ex = cx + side * headW * 0.22f - eyeW * 0.5f;
        g.setColour (glow);  g.fillEllipse (ex, eyeY, eyeW, eyeH);
        g.setColour (col);   g.drawEllipse (ex, eyeY, eyeW, eyeH, 1.5f);
        g.fillEllipse (ex + eyeW * 0.3f, eyeY + eyeH * 0.3f, eyeW * 0.35f, eyeH * 0.35f);
    }

    // Nose
    const float noseY = headTop + headH * 0.52f;
    juce::Path nose;
    nose.startNewSubPath (cx,           noseY - 4.5f*s);
    nose.lineTo          (cx - 3.5f*s,  noseY + 1.0f*s);
    nose.lineTo          (cx - 1.5f*s,  noseY + 1.0f*s);
    nose.startNewSubPath (cx,           noseY - 4.5f*s);
    nose.lineTo          (cx + 3.5f*s,  noseY + 1.0f*s);
    nose.lineTo          (cx + 1.5f*s,  noseY + 1.0f*s);
    g.setColour (col);
    g.strokePath (nose, juce::PathStrokeType (1.2f));

    // Teeth
    const float toothTop = jawTop + 1.0f*s, toothH = jawH * 0.46f;
    const float toothW   = jawW   * 0.17f,  toothGap = jawW * 0.215f;
    for (int i = -1; i <= 2; ++i)
    {
        float tx = cx - jawW * 0.5f + jawW * 0.1f + i * toothGap;
        g.fillRoundedRectangle (tx, toothTop, toothW, toothH, 1.5f);
    }

    // Skull crack
    juce::Path crack;
    crack.startNewSubPath (cx + 2.0f*s, headTop + headH * 0.08f);
    crack.lineTo          (cx - 3.0f*s, headTop + headH * 0.22f);
    crack.lineTo          (cx + 1.0f*s, headTop + headH * 0.34f);
    g.setColour (col.withAlpha (0.5f));
    g.strokePath (crack, juce::PathStrokeType (1.0f));

    // Cable
    juce::Path cable;
    cable.startNewSubPath (cx, hndY + hndH + 6.0f*s);
    cable.quadraticTo     (cx + 10.0f*s, hndY + hndH + 15.0f*s,
                           cx + 5.0f*s,  hndY + hndH + 22.0f*s);
    g.setColour (col.withAlpha (0.4f));
    g.strokePath (cable, juce::PathStrokeType (1.2f));
}

//==============================================================================
void ChordDetectEditor::drawChordPanel (juce::Graphics& g, juce::Rectangle<float> area)
{
    const bool active = lastChord.isValid && !lastNotes.empty();
    const juce::Colour col = active ? juce::Colour (COL_GREEN) : juce::Colour (COL_RED);

    g.setColour (juce::Colour (COL_PANEL));
    g.fillRoundedRectangle (area, 8.0f);
    g.setColour (col.withAlpha (0.30f));
    g.drawRoundedRectangle (area, 8.0f, 1.5f);

    auto inner = area.reduced (10.0f);

    if (!active)
    {
        g.setColour (juce::Colour (COL_RED));
        g.setFont   (juce::Font ("Helvetica Neue", 56.0f, juce::Font::plain));
        g.drawText  ("---", inner.removeFromTop (inner.getHeight() * 0.6f),
                     juce::Justification::centred);
        g.setColour (juce::Colour (COL_RED).withAlpha (0.5f));
        g.setFont   (juce::Font ("Helvetica Neue", 12.0f, juce::Font::italic));
        g.drawText  ("listening...", inner, juce::Justification::centred);
        return;
    }

    auto topArea = inner.removeFromTop (inner.getHeight() * 0.55f);
    g.setColour (juce::Colour (COL_GREEN).withAlpha (0.15f));
    g.setFont   (juce::Font ("Helvetica Neue", 72.0f, juce::Font::bold));
    g.drawText  (lastChord.root, topArea, juce::Justification::centred);
    g.setColour (juce::Colour (COL_GREEN));
    g.setFont   (juce::Font ("Helvetica Neue", 66.0f, juce::Font::bold));
    g.drawText  (lastChord.root, topArea, juce::Justification::centred);

    auto midArea = inner.removeFromTop (28.0f);
    if (lastChord.quality != "maj")
    {
        g.setColour (juce::Colour (COL_GREEN).withAlpha (0.80f));
        g.setFont   (juce::Font ("Helvetica Neue", 20.0f, juce::Font::plain));
        g.drawText  (lastChord.quality, midArea, juce::Justification::centred);
    }

    juce::String noteList;
    for (int pc : lastNotes) noteList += juce::String (NOTE_NAMES[pc]) + " ";
    g.setColour (juce::Colour (COL_DIM));
    g.setFont   (juce::Font ("Helvetica Neue", 12.0f, juce::Font::plain));
    g.drawText  (noteList.trim(), inner, juce::Justification::centred);
}

//==============================================================================
void ChordDetectEditor::drawPianoKeyboard (juce::Graphics& g,
                                            juce::Rectangle<float> area,
                                            const std::set<int>& active)
{
    const int   numOctaves = 2;
    const int   totalWhite = numOctaves * 7;
    const float wkW = area.getWidth()  / (float)totalWhite;
    const float wkH = area.getHeight();
    const float bkW = wkW * 0.58f;
    const float bkH = wkH * 0.60f;

    g.setColour (juce::Colour (COL_PANEL));
    g.fillRoundedRectangle (area.expanded (2.0f), 4.0f);

    for (int oct = 0; oct < numOctaves; ++oct)
    {
        for (int k = 0; k < 7; ++k)
        {
            const int   pc  = WHITE_KEYS[k];
            const bool  on  = active.count (pc) > 0;
            const float x   = area.getX() + (oct * 7 + k) * wkW;
            juce::Rectangle<float> r (x, area.getY(), wkW - 1.5f, wkH);

            if (on)
            {
                juce::ColourGradient grad (juce::Colour (COL_GREEN), r.getCentreX(), r.getY(),
                                            juce::Colour (COL_GREEN).darker (0.4f), r.getCentreX(), r.getBottom(), false);
                g.setGradientFill (grad);
            }
            else { g.setColour (juce::Colour (COL_WHITE)); }
            g.fillRoundedRectangle (r, 2.5f);
            g.setColour (on ? juce::Colour (COL_GREEN).darker (0.5f) : juce::Colour (COL_PANEL));
            g.drawRoundedRectangle (r, 2.5f, 1.0f);
        }
    }

    for (int oct = 0; oct < numOctaves; ++oct)
    {
        for (int k = 0; k < 7; ++k)
        {
            if (BLACK_KEYS[k] < 0) continue;
            const int   pc  = BLACK_KEYS[k];
            const bool  on  = active.count (pc) > 0;
            const float cx2 = area.getX() + (oct * 7 + k + 1) * wkW;
            juce::Rectangle<float> r (cx2 - bkW * 0.5f, area.getY(), bkW, bkH);

            if (on) { g.setColour (juce::Colour (COL_GREEN).darker (0.15f)); }
            else
            {
                juce::ColourGradient grad (juce::Colour (0xFF1A2E1A), r.getCentreX(), r.getY(),
                                            juce::Colour (COL_BG), r.getCentreX(), r.getBottom(), false);
                g.setGradientFill (grad);
            }
            g.fillRoundedRectangle (r, 2.5f);
            g.setColour (juce::Colour (COL_BG).withAlpha (0.6f));
            g.drawRoundedRectangle (r, 2.5f, 0.5f);
        }
    }
}
