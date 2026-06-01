#include "PluginEditor.h"

static const char* const NOTE_NAMES[12] = {
    "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
};

static const int WHITE_KEYS[7] = { 0, 2, 4, 5, 7, 9, 11 };
static const int BLACK_KEYS[7] = { 1, 3, -1, 6, 8, 10, -1 };

ChordDetectEditor::ChordDetectEditor (ChordDetectProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (500, 340);
    setResizable (true, true);
    setResizeLimits (400, 280, 960, 640);
    startTimerHz (20);
}

ChordDetectEditor::~ChordDetectEditor()
{
    stopTimer();
}

//==============================================================================
void ChordDetectEditor::paint (juce::Graphics& g)
{
    const bool active = lastChord.isValid && !lastNotes.empty();
    auto       bounds = getLocalBounds().toFloat();

    drawBackground (g);

    // ── Title bar ──────────────────────────────────────────────────────────
    auto titleBar = bounds.removeFromTop (46.0f);

    // Skull mic icon in title (small)
    auto iconArea = titleBar.removeFromLeft (46.0f).reduced (6.0f);
    drawSkullMic (g, iconArea, active);

    g.setColour (active ? juce::Colour (COL_GREEN) : juce::Colour (COL_RED));
    g.setFont   (juce::Font ("Helvetica Neue", 17.0f, juce::Font::bold));
    g.drawText  ("CHORDDETECT  VST3", titleBar, juce::Justification::centredLeft);

    // Separator
    g.setColour (juce::Colour (active ? COL_GREEN : COL_RED).withAlpha (0.25f));
    g.fillRect  (0.0f, 46.0f, (float)getWidth(), 1.0f);

    // ── Body — skull mic + chord panel side by side ─────────────────────
    auto body = bounds.withTrimmedBottom (105.0f);

    auto skullArea = body.removeFromLeft (body.getWidth() * 0.38f).reduced (10.0f, 6.0f);
    drawSkullMic  (g, skullArea, active);

    drawChordPanel (g, body.reduced (8.0f, 6.0f));

    // ── Piano keyboard ─────────────────────────────────────────────────
    auto piano = getLocalBounds().toFloat().removeFromBottom (100.0f).reduced (14.0f, 10.0f);
    drawPianoKeyboard (g, piano, lastNotes);
}

void ChordDetectEditor::resized() {}

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

    // Vignette glow from top — colour flips with state
    juce::Colour glowTop = active
        ? juce::Colour (COL_GREEN).withAlpha (0.10f)
        : juce::Colour (COL_RED)  .withAlpha (0.08f);

    juce::ColourGradient vignette (glowTop, b.getCentreX(), 0.0f,
                                    juce::Colour (COL_BG).withAlpha (0.0f),
                                    b.getCentreX(), b.getHeight() * 0.6f, true);
    g.setGradientFill (vignette);
    g.fillRect (b);

    // Subtle scanline texture
    g.setColour (juce::Colours::black.withAlpha (0.06f));
    for (float y = 0.0f; y < b.getHeight(); y += 3.0f)
        g.fillRect (0.0f, y, b.getWidth(), 1.0f);
}

//==============================================================================
void ChordDetectEditor::drawSkullMic (juce::Graphics& g,
                                       juce::Rectangle<float> area,
                                       bool active)
{
    const float cx  = area.getCentreX();
    const float cy  = area.getCentreY();
    const float h   = area.getHeight();
    const float s   = h / 100.0f;   // scale unit

    const juce::Colour col  = active ? juce::Colour (COL_GREEN) : juce::Colour (COL_RED);
    const juce::Colour glow = col.withAlpha (0.18f);
    const juce::Colour bg   = juce::Colour (COL_BG);

    // ── Mic handle (below skull) ──────────────────────────────────────────
    const float hndX  = cx - 8.0f * s;
    const float hndY  = cy + 16.0f * s;
    const float hndW  = 16.0f * s;
    const float hndH  = 24.0f * s;

    // Handle body
    g.setColour (col.withAlpha (0.75f));
    g.fillRoundedRectangle (hndX, hndY, hndW, hndH, 7.0f * s);

    // Grille lines across handle
    g.setColour (bg.withAlpha (0.45f));
    for (int i = 1; i <= 5; ++i)
        g.fillRect (hndX + 2.0f*s, hndY + i * (hndH / 6.0f), hndW - 4.0f*s, 1.0f);

    // Handle base / stand
    g.setColour (col.withAlpha (0.55f));
    g.fillRoundedRectangle (cx - 14.0f*s, hndY + hndH + 1.0f*s, 28.0f*s, 5.0f*s, 2.5f*s);

    // ── Skull (mic capsule head) ──────────────────────────────────────────
    const float headTop = cy - 40.0f * s;
    const float headW   = 38.0f * s;
    const float headH   = 36.0f * s;

    // Glow halo
    juce::Path halo;
    halo.addEllipse (cx - headW * 0.65f, headTop - headH * 0.1f, headW * 1.3f, headH * 1.2f);
    g.setColour (glow);
    g.fillPath (halo);

    // Cranium
    juce::Path cranium;
    cranium.addEllipse (cx - headW * 0.5f, headTop, headW, headH);

    g.setColour (bg);
    g.fillPath (cranium);
    g.setColour (col);
    g.strokePath (cranium, juce::PathStrokeType (1.8f));

    // Jaw / lower face (flatter oval)
    const float jawTop = headTop + headH * 0.62f;
    const float jawW   = headW  * 0.76f;
    const float jawH   = headH  * 0.46f;
    juce::Path jaw;
    jaw.addRoundedRectangle (cx - jawW * 0.5f, jawTop, jawW, jawH, jawW * 0.18f);

    g.setColour (bg);
    g.fillPath (jaw);
    g.setColour (col);
    g.strokePath (jaw, juce::PathStrokeType (1.5f));

    // Eyes — two hollow ovals
    const float eyeW = headW * 0.22f;
    const float eyeH = headH * 0.28f;
    const float eyeY = headTop + headH * 0.18f;

    for (int side : { -1, 1 })
    {
        float ex = cx + side * headW * 0.22f - eyeW * 0.5f;
        // Glow fill
        g.setColour (glow);
        g.fillEllipse (ex, eyeY, eyeW, eyeH);
        // Bright outline
        g.setColour (col);
        g.drawEllipse (ex, eyeY, eyeW, eyeH, 1.5f);
        // Inner dot
        g.fillEllipse (ex + eyeW * 0.3f, eyeY + eyeH * 0.3f, eyeW * 0.35f, eyeH * 0.35f);
    }

    // Nose — small inverted-V
    const float noseY = headTop + headH * 0.52f;
    juce::Path nose;
    nose.startNewSubPath (cx,              noseY - 4.5f*s);
    nose.lineTo          (cx - 3.5f*s,    noseY + 1.0f*s);
    nose.lineTo          (cx - 1.5f*s,    noseY + 1.0f*s);
    nose.startNewSubPath (cx,              noseY - 4.5f*s);
    nose.lineTo          (cx + 3.5f*s,    noseY + 1.0f*s);
    nose.lineTo          (cx + 1.5f*s,    noseY + 1.0f*s);
    g.setColour (col);
    g.strokePath (nose, juce::PathStrokeType (1.2f));

    // Teeth — 4 small rectangles along jaw top
    const float toothTop = jawTop + 1.0f*s;
    const float toothH   = jawH * 0.46f;
    const float toothW   = jawW * 0.17f;
    const float toothGap = jawW * 0.215f;

    g.setColour (col);
    for (int i = -1; i <= 2; ++i)
    {
        float tx = cx - jawW * 0.5f + jawW * 0.1f + i * toothGap;
        g.fillRoundedRectangle (tx, toothTop, toothW, toothH, 1.5f);
    }

    // Crack line across skull (decoration)
    juce::Path crack;
    crack.startNewSubPath (cx + 2.0f*s,  headTop + headH * 0.08f);
    crack.lineTo          (cx - 3.0f*s,  headTop + headH * 0.22f);
    crack.lineTo          (cx + 1.0f*s,  headTop + headH * 0.34f);
    g.setColour (col.withAlpha (0.5f));
    g.strokePath (crack, juce::PathStrokeType (1.0f));

    // Wire / cable from mic bottom
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

    // Panel bg
    g.setColour (juce::Colour (COL_PANEL));
    g.fillRoundedRectangle (area, 8.0f);
    g.setColour (col.withAlpha (0.30f));
    g.drawRoundedRectangle (area, 8.0f, 1.5f);

    auto inner = area.reduced (10.0f);

    if (!active)
    {
        // Big red dashes
        g.setColour (juce::Colour (COL_RED));
        g.setFont   (juce::Font ("Helvetica Neue", 56.0f, juce::Font::plain));
        g.drawText  ("---", inner.removeFromTop (inner.getHeight() * 0.6f),
                     juce::Justification::centred);

        g.setColour (juce::Colour (COL_RED).withAlpha (0.5f));
        g.setFont   (juce::Font ("Helvetica Neue", 12.0f, juce::Font::italic));
        g.drawText  ("listening...", inner, juce::Justification::centred);
        return;
    }

    // Root note — large neon green
    auto topArea = inner.removeFromTop (inner.getHeight() * 0.55f);

    g.setColour (juce::Colour (COL_GREEN).withAlpha (0.15f));  // glow
    g.setFont   (juce::Font ("Helvetica Neue", 72.0f, juce::Font::bold));
    g.drawText  (lastChord.root, topArea, juce::Justification::centred);

    g.setColour (juce::Colour (COL_GREEN));                    // crisp
    g.setFont   (juce::Font ("Helvetica Neue", 66.0f, juce::Font::bold));
    g.drawText  (lastChord.root, topArea, juce::Justification::centred);

    // Quality tag
    auto midArea = inner.removeFromTop (28.0f);
    if (lastChord.quality != "maj")
    {
        g.setColour (juce::Colour (COL_GREEN).withAlpha (0.80f));
        g.setFont   (juce::Font ("Helvetica Neue", 20.0f, juce::Font::plain));
        g.drawText  (lastChord.quality, midArea, juce::Justification::centred);
    }

    // Individual notes
    juce::String noteList;
    for (int pc : lastNotes)
        noteList += juce::String (NOTE_NAMES[pc]) + " ";

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

    // Keyboard outline
    g.setColour (juce::Colour (COL_PANEL));
    g.fillRoundedRectangle (area.expanded (2.0f), 4.0f);

    // White keys
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
                juce::ColourGradient grad (juce::Colour (COL_GREEN),
                                            r.getCentreX(), r.getY(),
                                            juce::Colour (COL_GREEN).darker (0.4f),
                                            r.getCentreX(), r.getBottom(), false);
                g.setGradientFill (grad);
            }
            else
            {
                g.setColour (juce::Colour (COL_WHITE));
            }
            g.fillRoundedRectangle (r, 2.5f);
            g.setColour (on ? juce::Colour (COL_GREEN).darker (0.5f)
                            : juce::Colour (COL_PANEL));
            g.drawRoundedRectangle (r, 2.5f, 1.0f);
        }
    }

    // Black keys
    for (int oct = 0; oct < numOctaves; ++oct)
    {
        for (int k = 0; k < 7; ++k)
        {
            if (BLACK_KEYS[k] < 0) continue;
            const int   pc  = BLACK_KEYS[k];
            const bool  on  = active.count (pc) > 0;
            const float cx2 = area.getX() + (oct * 7 + k + 1) * wkW;
            juce::Rectangle<float> r (cx2 - bkW * 0.5f, area.getY(), bkW, bkH);

            if (on)
            {
                g.setColour (juce::Colour (COL_GREEN).darker (0.15f));
            }
            else
            {
                juce::ColourGradient grad (juce::Colour (0xFF1A2E1A),
                                            r.getCentreX(), r.getY(),
                                            juce::Colour (COL_BG),
                                            r.getCentreX(), r.getBottom(), false);
                g.setGradientFill (grad);
            }
            g.fillRoundedRectangle (r, 2.5f);
            g.setColour (juce::Colour (COL_BG).withAlpha (0.6f));
            g.drawRoundedRectangle (r, 2.5f, 0.5f);
        }
    }
}
