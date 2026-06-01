#include "PluginEditor.h"

static const char* const NOTE_NAMES[12] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
const int ChordDetectEditor::WHITE_KEYS[7] = {0,2,4,5,7,9,11};
const int ChordDetectEditor::BLACK_KEYS[7] = {1,3,-1,6,8,10,-1};

//==============================================================================
// Virtual keyboard helpers
//==============================================================================
int ChordDetectEditor::pianoNoteAtPoint (juce::Point<int> pos) const
{
    auto area = lay.pianoKeys.toFloat().reduced (10.f, 6.f);
    if (!area.contains (pos.toFloat())) return -1;

    const int numOctaves = 2;
    const float wkW = area.getWidth() / (float)(numOctaves * 7);
    const float bkW = wkW * 0.58f;
    const float bkH = area.getHeight() * 0.60f;
    const float relX = (float)pos.x - area.getX();
    const float relY = (float)pos.y - area.getY();

    // Check black keys first (drawn on top)
    if (relY < bkH)
    {
        for (int oct = 0; oct < numOctaves; ++oct)
            for (int k = 0; k < 7; ++k)
            {
                if (BLACK_KEYS[k] < 0) continue;
                float cx = (oct * 7 + k + 1) * wkW;
                if (relX >= cx - bkW * 0.5f && relX < cx + bkW * 0.5f)
                    return keyboardBaseNote + oct * 12 + BLACK_KEYS[k];
            }
    }

    // White keys
    int keyIdx = (int)(relX / wkW);
    if (keyIdx >= 0 && keyIdx < numOctaves * 7)
    {
        int oct = keyIdx / 7;
        int k   = keyIdx % 7;
        return keyboardBaseNote + oct * 12 + WHITE_KEYS[k];
    }
    return -1;
}

void ChordDetectEditor::virtualNoteOn (int note)
{
    if (note < 0 || note > 127) return;
    proc.keyboardState.noteOn (1, note, 0.8f);
    heldVirtualNote = note;
    repaint (lay.pianoKeys);
}

void ChordDetectEditor::virtualNoteOff (int note)
{
    if (note < 0 || note > 127) return;
    proc.keyboardState.noteOff (1, note, 0.0f);
    heldVirtualNote = -1;
    repaint (lay.pianoKeys);
}

void ChordDetectEditor::mouseDown (const juce::MouseEvent& e)
{
    int note = pianoNoteAtPoint (e.getPosition());
    if (note >= 0) virtualNoteOn (note);
}

void ChordDetectEditor::mouseUp (const juce::MouseEvent& e)
{
    juce::ignoreUnused (e);
    if (heldVirtualNote >= 0) virtualNoteOff (heldVirtualNote);
}

void ChordDetectEditor::mouseDrag (const juce::MouseEvent& e)
{
    int note = pianoNoteAtPoint (e.getPosition());
    if (note != heldVirtualNote)
    {
        if (heldVirtualNote >= 0) virtualNoteOff (heldVirtualNote);
        if (note >= 0)            virtualNoteOn  (note);
    }
}

//==============================================================================
// Helpers
//==============================================================================
void ChordDetectEditor::styleBtn (juce::TextButton& b, bool on, juce::uint32 onCol)
{
    juce::Colour c(onCol);
    b.setColour (juce::TextButton::buttonColourId,  on ? c.withAlpha(0.22f) : juce::Colour(C_PANEL));
    b.setColour (juce::TextButton::buttonOnColourId, c.withAlpha(0.32f));
    b.setColour (juce::TextButton::textColourOffId,  on ? c : juce::Colour(C_DIM));
    b.setColour (juce::TextButton::textColourOnId,   c);
}

void ChordDetectEditor::addSlider (juce::Slider& s, juce::Slider::SliderStyle style,
                                    juce::uint32 col)
{
    s.setSliderStyle (style);
    bool rotary = (style == juce::Slider::Rotary || style == juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (rotary ? juce::Slider::NoTextBox : juce::Slider::TextBoxRight,
                       false, rotary ? 0 : 36, 16);
    s.setColour (juce::Slider::thumbColourId,             juce::Colour(col));
    s.setColour (juce::Slider::rotarySliderFillColourId,  juce::Colour(col).withAlpha(0.7f));
    s.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour(C_PANEL));
    s.setColour (juce::Slider::trackColourId,             juce::Colour(col).withAlpha(0.4f));
    s.setColour (juce::Slider::backgroundColourId,        juce::Colour(C_PANEL));
    s.setColour (juce::Slider::textBoxTextColourId,       juce::Colour(col));
    s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour(C_BG));
    s.setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
    addAndMakeVisible (s);
}

//==============================================================================
// Setup helpers
//==============================================================================
void ChordDetectEditor::setupLatencyBtns()
{
    int cur = (int)proc.apvts.getRawParameterValue(ChordDetectProcessor::PID_LATENCY)->load();
    juce::TextButton* btns[3] = { &latLowBtn, &latBalBtn, &latHiBtn };
    for (int i = 0; i < 3; ++i)
    {
        styleBtn (*btns[i], i == cur);
        btns[i]->onClick = [this, i]()
        {
            auto* p = dynamic_cast<juce::AudioParameterChoice*>(
                proc.apvts.getParameter(ChordDetectProcessor::PID_LATENCY));
            if (p) p->setValueNotifyingHost(p->convertTo0to1((float)i));
            styleBtn(latLowBtn, i==0); styleBtn(latBalBtn, i==1); styleBtn(latHiBtn, i==2);
        };
        addAndMakeVisible (*btns[i]);
    }
}

void ChordDetectEditor::setupHarmonyBtns()
{
    int cur = (int)proc.apvts.getRawParameterValue(ChordDetectProcessor::PID_HARMONY)->load();
    juce::TextButton* btns[5] = {&harmOffBtn,&harm3Btn,&harm5Btn,&harmOctBtn,&harmChordBtn};
    for (int i = 0; i < 5; ++i)
    {
        styleBtn (*btns[i], i == cur, C_AMBER);
        btns[i]->onClick = [this, i]()
        {
            auto* p = dynamic_cast<juce::AudioParameterChoice*>(
                proc.apvts.getParameter(ChordDetectProcessor::PID_HARMONY));
            if (p) p->setValueNotifyingHost(p->convertTo0to1((float)i));
            juce::TextButton* b2[5]={&harmOffBtn,&harm3Btn,&harm5Btn,&harmOctBtn,&harmChordBtn};
            for (int j=0;j<5;++j) styleBtn(*b2[j], j==i, C_AMBER);
        };
        addAndMakeVisible (*btns[i]);
    }
}

void ChordDetectEditor::setupSuggestionBtns()
{
    for (int i = 0; i < 5; ++i)
    {
        styleBtn (suggBtn[i], false, C_CYAN);
        suggBtn[i].onClick = [this, i]()
        {
            juce::MidiBuffer dummy;
            proc.playSuggestion (i, dummy);
        };
        addAndMakeVisible (suggBtn[i]);
    }
}

//==============================================================================
// Constructor
//==============================================================================
ChordDetectEditor::ChordDetectEditor (ChordDetectProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setSize (580, 700);
    setResizable (true, true);
    setResizeLimits (480, 600, 1100, 1000);

    // Pitch slider
    addSlider (pitchSlider, juce::Slider::LinearHorizontal, C_GREEN);
    pitchAtt = std::make_unique<SA>(proc.apvts, ChordDetectProcessor::PID_PITCH, pitchSlider);

    // Voice sens
    addSlider (voiceSensSlider, juce::Slider::LinearHorizontal, C_AMBER);
    voiceSensAtt = std::make_unique<SA>(proc.apvts, ChordDetectProcessor::PID_VOICE_SENS, voiceSensSlider);

    // Voice on button
    bool von = proc.apvts.getRawParameterValue(ChordDetectProcessor::PID_VOICE_ON)->load()>0.5f;
    styleBtn (voiceOnBtn, von, C_AMBER);
    voiceOnBtn.onClick = [this]() {
        auto* pb = dynamic_cast<juce::AudioParameterBool*>(
            proc.apvts.getParameter(ChordDetectProcessor::PID_VOICE_ON));
        if (pb) pb->setValueNotifyingHost(pb->get()?0.f:1.f);
        styleBtn(voiceOnBtn,
            proc.apvts.getRawParameterValue(ChordDetectProcessor::PID_VOICE_ON)->load()>0.5f,
            C_AMBER);
    };
    addAndMakeVisible (voiceOnBtn);

    // EQ gain knobs (rotary) + frequency knobs (rotary, cyan tint)
    addSlider (eqLowSlider,    juce::Slider::RotaryHorizontalVerticalDrag, C_GREEN);
    addSlider (eqLowHzSlider,  juce::Slider::RotaryHorizontalVerticalDrag, C_CYAN);
    addSlider (eqMidSlider,    juce::Slider::RotaryHorizontalVerticalDrag, C_GREEN);
    addSlider (eqMidHzSlider,  juce::Slider::RotaryHorizontalVerticalDrag, C_CYAN);
    addSlider (eqHighSlider,   juce::Slider::RotaryHorizontalVerticalDrag, C_GREEN);
    addSlider (eqHighHzSlider, juce::Slider::RotaryHorizontalVerticalDrag, C_CYAN);
    eqLowAtt    = std::make_unique<SA>(proc.apvts, ChordDetectProcessor::PID_EQ_LOW,     eqLowSlider);
    eqLowHzAtt  = std::make_unique<SA>(proc.apvts, ChordDetectProcessor::PID_EQ_LOW_HZ,  eqLowHzSlider);
    eqMidAtt    = std::make_unique<SA>(proc.apvts, ChordDetectProcessor::PID_EQ_MID,     eqMidSlider);
    eqMidHzAtt  = std::make_unique<SA>(proc.apvts, ChordDetectProcessor::PID_EQ_MID_HZ,  eqMidHzSlider);
    eqHighAtt   = std::make_unique<SA>(proc.apvts, ChordDetectProcessor::PID_EQ_HIGH,    eqHighSlider);
    eqHighHzAtt = std::make_unique<SA>(proc.apvts, ChordDetectProcessor::PID_EQ_HIGH_HZ, eqHighHzSlider);

    // Reverb sliders
    addSlider (revRoomSlider, juce::Slider::RotaryHorizontalVerticalDrag, C_CYAN);
    addSlider (revWetSlider,  juce::Slider::RotaryHorizontalVerticalDrag, C_CYAN);
    revRoomAtt = std::make_unique<SA>(proc.apvts, ChordDetectProcessor::PID_REV_ROOM, revRoomSlider);
    revWetAtt  = std::make_unique<SA>(proc.apvts, ChordDetectProcessor::PID_REV_WET,  revWetSlider);

    // Delay sliders
    addSlider (dlyTimeSlider, juce::Slider::RotaryHorizontalVerticalDrag, C_AMBER);
    addSlider (dlyFdbkSlider, juce::Slider::RotaryHorizontalVerticalDrag, C_AMBER);
    addSlider (dlyWetSlider,  juce::Slider::RotaryHorizontalVerticalDrag, C_AMBER);
    dlyTimeAtt = std::make_unique<SA>(proc.apvts, ChordDetectProcessor::PID_DLY_TIME, dlyTimeSlider);
    dlyFdbkAtt = std::make_unique<SA>(proc.apvts, ChordDetectProcessor::PID_DLY_FDBK, dlyFdbkSlider);
    dlyWetAtt  = std::make_unique<SA>(proc.apvts, ChordDetectProcessor::PID_DLY_WET,  dlyWetSlider);

    // Latency / Harmony buttons
    setupLatencyBtns();
    setupHarmonyBtns();

    // MIDI recorder buttons
    bool recOn = proc.apvts.getRawParameterValue(ChordDetectProcessor::PID_MIDI_REC)->load()>0.5f;
    styleBtn (midiRecBtn, recOn, C_RED);
    midiRecBtn.onClick = [this]() {
        auto* pb = dynamic_cast<juce::AudioParameterBool*>(
            proc.apvts.getParameter(ChordDetectProcessor::PID_MIDI_REC));
        if (pb) pb->setValueNotifyingHost(pb->get()?0.f:1.f);
        styleBtn(midiRecBtn,
            proc.apvts.getRawParameterValue(ChordDetectProcessor::PID_MIDI_REC)->load()>0.5f,
            C_RED);
    };
    styleBtn(midiClrBtn,false,C_DIM); midiClrBtn.onClick=[this](){ proc.clearRecording(); };
    styleBtn(midiQntBtn,false,C_DIM); midiQntBtn.onClick=[this](){ proc.quantizeRecording(); };
    styleBtn(midiM12Btn,false,C_DIM); midiM12Btn.onClick=[this](){ proc.transposeRecording(-12); };
    styleBtn(midiM1Btn, false,C_DIM); midiM1Btn .onClick=[this](){ proc.transposeRecording(-1); };
    styleBtn(midiP1Btn, false,C_DIM); midiP1Btn .onClick=[this](){ proc.transposeRecording(1); };
    styleBtn(midiP12Btn,false,C_DIM); midiP12Btn.onClick=[this](){ proc.transposeRecording(12); };
    for (auto* b : {&midiRecBtn,&midiClrBtn,&midiQntBtn,&midiM12Btn,&midiM1Btn,&midiP1Btn,&midiP12Btn})
        addAndMakeVisible(*b);

    // AI suggestion buttons
    setupSuggestionBtns();

    // Octave shift buttons
    styleBtn (octDownBtn, false, C_DIM);
    styleBtn (octUpBtn,   false, C_DIM);
    octDownBtn.onClick = [this]() {
        keyboardBaseNote = juce::jmax (0,  keyboardBaseNote - 12);
        repaint (lay.pianoKeys);
    };
    octUpBtn.onClick = [this]() {
        keyboardBaseNote = juce::jmin (96, keyboardBaseNote + 12);
        repaint (lay.pianoKeys);
    };
    addAndMakeVisible (octDownBtn);
    addAndMakeVisible (octUpBtn);

    startTimerHz (15);
}

ChordDetectEditor::~ChordDetectEditor() { stopTimer(); }

//==============================================================================
// Layout
//==============================================================================
void ChordDetectEditor::resized()
{
    const int W   = getWidth();
    const int H   = getHeight();
    const int pad = 10;

    int y = 0;
    lay.title    = {0, y, W, 46};       y += 46;
    lay.body     = {0, y, W, 140};      y += 140;
    lay.strip1   = {0, y, W, 38};       y += 38;  // pitch + latency
    lay.strip2   = {0, y, W, 38};       y += 38;  // harmony
    lay.strip3   = {0, y, W, 38};       y += 38;  // voice
    lay.eqStrip  = {0, y, W, 82};       y += 82;  // EQ (taller for 2 rows of labels)
    lay.fxStrip  = {0, y, W, 72};       y += 72;  // Reverb + Delay
    lay.aiStrip  = {0, y, W, 48};       y += 48;  // AI suggest
    lay.midiCtrl = {0, y, W, 34};       y += 34;  // MIDI controls
    lay.pianoRoll= {0, y, W, H - y - 80}; y = H - 80;
    lay.pianoKeys= {0, y, W, 88};

    // Strip 1: pitch slider + latency buttons
    {
        auto r = lay.strip1.reduced(pad, 6);
        int labelW = 90, btnW = 38;
        int sliderW = r.getWidth()/2 - labelW - 4;
        pitchSlider.setBounds(r.getX()+labelW, r.getY(), sliderW, r.getHeight());
        int bx = r.getX() + r.getWidth()/2 + labelW;
        latLowBtn.setBounds(bx,           r.getY(), btnW, r.getHeight());
        latBalBtn.setBounds(bx+btnW+2,    r.getY(), btnW, r.getHeight());
        latHiBtn .setBounds(bx+btnW*2+4,  r.getY(), btnW, r.getHeight());
    }
    // Strip 2: harmony buttons
    {
        auto r = lay.strip2.reduced(pad, 6);
        int labelW = 70;
        int bw = (r.getWidth()-labelW) / 5;
        juce::TextButton* hb[5]={&harmOffBtn,&harm3Btn,&harm5Btn,&harmOctBtn,&harmChordBtn};
        for (int i=0;i<5;++i)
            hb[i]->setBounds(r.getX()+labelW+i*bw, r.getY(), bw-2, r.getHeight());
    }
    // Strip 3: voice button + sens slider
    {
        auto r = lay.strip3.reduced(pad, 6);
        int btnW = 108;
        voiceOnBtn    .setBounds(r.getX(),         r.getY(), btnW,                  r.getHeight());
        voiceSensSlider.setBounds(r.getX()+btnW+50, r.getY(), r.getWidth()-btnW-52, r.getHeight());
    }
    // EQ strip: 3 bands, each with gain knob + freq knob
    {
        auto r = lay.eqStrip.reduced(pad, 4);
        int labelW = 52, knobW = 48, knobH = r.getHeight() - 4;
        int third = r.getWidth() / 3;
        // Bass
        eqLowSlider  .setBounds(r.getX()          + labelW,         r.getY(), knobW, knobH);
        eqLowHzSlider.setBounds(r.getX()          + labelW + knobW, r.getY(), knobW, knobH);
        // Mid
        eqMidSlider  .setBounds(r.getX() + third  + labelW,         r.getY(), knobW, knobH);
        eqMidHzSlider.setBounds(r.getX() + third  + labelW + knobW, r.getY(), knobW, knobH);
        // High
        eqHighSlider .setBounds(r.getX() + third*2 + labelW,         r.getY(), knobW, knobH);
        eqHighHzSlider.setBounds(r.getX()+ third*2 + labelW + knobW, r.getY(), knobW, knobH);
    }
    // FX strip: reverb (2 knobs) + delay (3 knobs)
    {
        auto r = lay.fxStrip.reduced(pad, 4);
        int knobW = 52, labelW = 52;
        int half = r.getWidth()/2;
        revRoomSlider.setBounds(r.getX()      +labelW,    r.getY(), knobW, r.getHeight());
        revWetSlider .setBounds(r.getX()      +labelW+56, r.getY(), knobW, r.getHeight());
        dlyTimeSlider.setBounds(r.getX()+half +labelW,    r.getY(), knobW, r.getHeight());
        dlyFdbkSlider.setBounds(r.getX()+half +labelW+56, r.getY(), knobW, r.getHeight());
        dlyWetSlider .setBounds(r.getX()+half +labelW+112,r.getY(), knobW, r.getHeight());
    }
    // AI strip: 5 suggestion buttons
    {
        auto r = lay.aiStrip.reduced(pad, 6);
        int labelW = 70, bw = (r.getWidth()-labelW)/5;
        for (int i=0;i<5;++i)
            suggBtn[i].setBounds(r.getX()+labelW+i*bw, r.getY(), bw-2, r.getHeight());
    }
    // Octave buttons — sit inside the piano key area, top-left corner
    {
        int bx = lay.pianoKeys.getX() + pad;
        int by = lay.pianoKeys.getY() + 4;
        octDownBtn.setBounds (bx,       by, 46, 18);
        octUpBtn  .setBounds (bx + 48,  by, 46, 18);
    }

    // MIDI control bar
    {
        auto r = lay.midiCtrl.reduced(pad, 5);
        int bw = 42;
        midiRecBtn.setBounds(r.getX(),          r.getY(), 54,   r.getHeight());
        midiClrBtn.setBounds(r.getX()+58,       r.getY(), bw,   r.getHeight());
        midiQntBtn.setBounds(r.getX()+58+bw+2,  r.getY(), bw,   r.getHeight());
        int tx = r.getRight() - bw*4 - 6;
        midiM12Btn.setBounds(tx,          r.getY(), bw, r.getHeight());
        midiM1Btn .setBounds(tx+bw+2,     r.getY(), bw, r.getHeight());
        midiP1Btn .setBounds(tx+bw*2+4,   r.getY(), bw, r.getHeight());
        midiP12Btn.setBounds(tx+bw*3+6,   r.getY(), bw, r.getHeight());
    }
}

//==============================================================================
// Paint
//==============================================================================
void ChordDetectEditor::paint (juce::Graphics& g)
{
    const bool active = lastChord.isValid && !lastNotes.empty();
    paintBackground (g);
    paintTitleBar   (g, lay.title);
    paintChordBody  (g, lay.body);
    paintSectionLabel(g, lay.strip1, "PITCH / LATENCY", C_GREEN);
    paintSectionLabel(g, lay.strip2, "HARMONY",         C_AMBER);
    paintSectionLabel(g, lay.strip3, "MIC > MIDI  SENS", C_AMBER);
    paintSectionLabel(g, lay.eqStrip, "EQ",             C_GREEN);
    paintEQLabels   (g);
    paintFXLabels   (g);
    paintSectionLabel(g, lay.aiStrip,  "AI SUGGEST",    C_CYAN);
    paintSectionLabel(g, lay.midiCtrl, "MIDI",          C_RED);
    paintPianoRoll  (g, lay.pianoRoll);
    paintPianoKeys  (g, lay.pianoKeys);
    juce::ignoreUnused(active);
}

void ChordDetectEditor::paintEQLabels (juce::Graphics& g)
{
    auto r  = lay.eqStrip.reduced(10, 4);
    int third = r.getWidth() / 3;

    // Band names + sub-labels
    struct Band { const char* name; const char* col1; const char* col2; juce::uint32 colour; };
    Band bands[3] = {
        { "BASS",     "Gain", "Freq", C_GREEN },
        { "MID FREQ", "Gain", "Freq", C_GREEN },
        { "HI FREQ",  "Gain", "Freq", C_GREEN },
    };

    int labelW = 52, knobW = 48;
    for (int i = 0; i < 3; ++i)
    {
        int bx = r.getX() + third * i;
        // Band name
        g.setColour(juce::Colour(bands[i].colour));
        g.setFont({10.f, juce::Font::bold});
        g.drawText(bands[i].name, bx, r.getY(), labelW, r.getHeight()/2,
                   juce::Justification::centredLeft);
        // Sub-labels under knobs
        g.setColour(juce::Colour(C_DIM));
        g.setFont({9.f});
        g.drawText("Gain", bx + labelW,         r.getBottom() - 14, knobW, 14, juce::Justification::centred);
        g.drawText("Hz",   bx + labelW + knobW, r.getBottom() - 14, knobW, 14, juce::Justification::centred);
    }
}

void ChordDetectEditor::paintFXLabels (juce::Graphics& g)
{
    paintSectionLabel(g, lay.fxStrip, "FX", C_CYAN);
    auto r = lay.fxStrip.reduced(10, 4);
    int half = r.getWidth()/2;
    g.setColour(juce::Colour(C_DIM)); g.setFont({10.f});
    g.drawText("REVERB", r.getX()+0,    r.getY(), 48, 16, juce::Justification::centredLeft);
    g.drawText("Room",   r.getX()+52,   r.getBottom()-14, 48, 14, juce::Justification::centred);
    g.drawText("Wet",    r.getX()+108,  r.getBottom()-14, 48, 14, juce::Justification::centred);
    g.drawText("DELAY",  r.getX()+half, r.getY(), 48, 16, juce::Justification::centredLeft);
    g.drawText("Time",   r.getX()+half+52,  r.getBottom()-14, 48, 14, juce::Justification::centred);
    g.drawText("Fdbk",   r.getX()+half+108, r.getBottom()-14, 48, 14, juce::Justification::centred);
    g.drawText("Wet",    r.getX()+half+164, r.getBottom()-14, 48, 14, juce::Justification::centred);
}

void ChordDetectEditor::timerCallback()
{
    auto chord = proc.getCurrentChord();
    auto notes = proc.getCurrentNotes();
    int  vn    = proc.getCurrentVoiceNote();
    auto suggs = proc.getSuggestions();

    bool changed = (chord.name != lastChord.name) || (notes != lastNotes)
                || (vn != lastVoiceNote) || (suggs.size() != lastSuggestions.size());

    lastChord = chord; lastNotes = notes; lastVoiceNote = vn; lastSuggestions = suggs;

    // Update suggestion button labels
    for (int i = 0; i < 5; ++i)
    {
        if (i < (int)suggs.size())
        {
            suggBtn[i].setButtonText (suggs[i].name);
            suggBtn[i].setVisible (true);
        }
        else suggBtn[i].setVisible (false);
    }

    if (changed) repaint();
}

//==============================================================================
void ChordDetectEditor::paintBackground (juce::Graphics& g)
{
    const bool active = lastChord.isValid && !lastNotes.empty();
    g.fillAll (juce::Colour(C_BG));

    // Subtle top glow
    juce::Colour gc = active ? juce::Colour(C_GREEN).withAlpha(0.08f)
                             : juce::Colour(C_RED)  .withAlpha(0.06f);
    juce::ColourGradient grad(gc, getWidth()*0.5f, 0,
        juce::Colour(C_BG).withAlpha(0.f), getWidth()*0.5f, 220, true);
    g.setGradientFill(grad); g.fillRect(getLocalBounds());

    // Scanlines
    g.setColour(juce::Colours::black.withAlpha(0.04f));
    for (int y=0; y<getHeight(); y+=3) g.fillRect(0,y,getWidth(),1);

    // Section dividers
    g.setColour(juce::Colour(C_PANEL).withAlpha(0.8f));
    for (int divY : { lay.body.getBottom(), lay.strip1.getBottom(), lay.strip2.getBottom(),
                      lay.strip3.getBottom(), lay.eqStrip.getBottom(), lay.fxStrip.getBottom(),
                      lay.aiStrip.getBottom(), lay.midiCtrl.getBottom(), lay.pianoRoll.getBottom() })
        g.fillRect(10, divY, getWidth()-20, 1);
}

void ChordDetectEditor::paintSectionLabel (juce::Graphics& g, juce::Rectangle<int> area,
                                            const juce::String& label, juce::uint32 col)
{
    g.setColour (juce::Colour(col).withAlpha(0.75f));
    g.setFont   ({9.5f, juce::Font::bold});
    g.drawText  (label, area.getX()+10, area.getY(), 85, area.getHeight(),
                 juce::Justification::centredLeft);
}

//==============================================================================
void ChordDetectEditor::paintTitleBar (juce::Graphics& g, juce::Rectangle<int> area)
{
    const bool active = lastChord.isValid && !lastNotes.empty();
    juce::Colour col = active ? juce::Colour(C_GREEN) : juce::Colour(C_RED);

    // Skull mic (left 46px)
    paintSkullMic (g, juce::Rectangle<float>(0,0,46,46), active);

    // Johnny Nix logo
    paintJohnnyNixLogo (g, juce::Rectangle<float>(46,0,170,46), col);

    // Subtitle
    g.setColour (col.withAlpha(0.55f));
    g.setFont   ({10.5f});
    g.drawText  ("ChordDetect VST3  |  Harmony  |  Voice > MIDI  |  EQ  |  FX  |  AI",
                 220, 0, area.getWidth()-226, 46, juce::Justification::centredLeft);

    // Bottom separator
    g.setColour (col.withAlpha(0.2f));
    g.fillRect  (0, 45, area.getWidth(), 1);
}

//==============================================================================
void ChordDetectEditor::paintChordBody (juce::Graphics& g, juce::Rectangle<int> area)
{
    auto fa = area.toFloat();
    auto skullArea = fa.removeFromLeft(fa.getWidth() * 0.35f).reduced(8,6);
    paintSkullMic     (g, skullArea, lastChord.isValid && !lastNotes.empty());
    paintChordDisplay (g, fa.reduced(6,6));
}

void ChordDetectEditor::paintChordDisplay (juce::Graphics& g, juce::Rectangle<float> area)
{
    const bool active = lastChord.isValid && !lastNotes.empty();
    juce::Colour col = active ? juce::Colour(C_GREEN) : juce::Colour(C_RED);

    g.setColour(juce::Colour(C_PANEL)); g.fillRoundedRectangle(area, 8);
    g.setColour(col.withAlpha(0.28f));  g.drawRoundedRectangle(area, 8, 1.5f);

    auto inner = area.reduced(10);
    if (!active)
    {
        g.setColour(juce::Colour(C_RED));
        g.setFont({48.f}); g.drawText("---", inner.removeFromTop(inner.getHeight()*0.58f), juce::Justification::centred);
        g.setColour(juce::Colour(C_RED).withAlpha(0.45f));
        g.setFont({11.f,juce::Font::italic}); g.drawText("listening...", inner, juce::Justification::centred);
        return;
    }
    auto top = inner.removeFromTop(inner.getHeight()*0.56f);
    g.setColour(juce::Colour(C_GREEN).withAlpha(0.12f));
    g.setFont({60.f,juce::Font::bold}); g.drawText(lastChord.root, top, juce::Justification::centred);
    g.setColour(juce::Colour(C_GREEN));
    g.setFont({55.f,juce::Font::bold}); g.drawText(lastChord.root, top, juce::Justification::centred);

    auto mid = inner.removeFromTop(24.f);
    if (lastChord.quality != "maj")
    {
        g.setColour(juce::Colour(C_GREEN).withAlpha(0.75f));
        g.setFont({17.f}); g.drawText(lastChord.quality, mid, juce::Justification::centred);
    }

    // Voice note indicator
    if (lastVoiceNote >= 0)
    {
        juce::String vStr = juce::String(NOTE_NAMES[lastVoiceNote%12]) + juce::String(lastVoiceNote/12-1);
        g.setColour(juce::Colour(C_AMBER));
        g.setFont({12.f,juce::Font::bold});
        g.drawText("note: " + vStr, inner.removeFromTop(16), juce::Justification::centredRight);
    }

    juce::String nl; for(int pc:lastNotes) nl+=juce::String(NOTE_NAMES[pc])+" ";
    g.setColour(juce::Colour(C_DIM)); g.setFont({11.f});
    g.drawText(nl.trim(), inner, juce::Justification::centred);
}

//==============================================================================
void ChordDetectEditor::paintPianoRoll (juce::Graphics& g, juce::Rectangle<int> area)
{
    auto fa = area.toFloat();
    g.setColour (juce::Colour (C_PANEL));
    g.fillRoundedRectangle (fa, 4.f);

    auto events = proc.getRecordedEvents();

    // ── Note axis width ──────────────────────────────────────────────
    const float labelW = 28.f;   // left column for note names

    if (events.empty())
    {
        // Draw a mini keyboard axis even with no events
        g.setColour (juce::Colour (C_DIM).withAlpha (0.35f));
        g.setFont ({ 9.f });
        g.drawText ("no MIDI recorded — press REC and play",
                    fa.withTrimmedLeft (labelW), juce::Justification::centred);

        // Draw empty axis
        for (int octave = 1; octave <= 7; ++octave)
        {
            int note = octave * 12;   // C of each octave
            float normY = 1.f - (float)(note - 12) / (float)(7 * 12);
            float y = fa.getY() + normY * fa.getHeight();
            g.setColour (juce::Colour (C_DIM).withAlpha (0.25f));
            g.drawHorizontalLine ((int)y, fa.getX() + labelW, fa.getRight());
            g.setColour (juce::Colour (C_DIM).withAlpha (0.55f));
            g.drawText (juce::String ("C") + juce::String (octave),
                        (int)fa.getX(), (int)(y - 7.f), (int)labelW - 2, 14,
                        juce::Justification::centredRight);
        }
        return;
    }

    // ── Determine note range ─────────────────────────────────────────
    int minNote = 127, maxNote = 0;
    for (auto& ev : events)
    {
        minNote = juce::jmin (minNote, ev.note);
        maxNote = juce::jmax (maxNote, ev.note);
    }
    minNote = juce::jmax (0,   minNote - 3);
    maxNote = juce::jmin (127, maxNote + 3);
    int noteRange = juce::jmax (1, maxNote - minNote);

    // ── Time window ──────────────────────────────────────────────────
    const double totalTime = proc.getRecorderTime();
    const double windowSec = juce::jmax (4.0, totalTime);
    const double startTime = totalTime - windowSec;

    // Content area (right of note axis)
    auto content = fa.withTrimmedLeft (labelW);
    const float cW = content.getWidth();
    const float cH = content.getHeight();
    const float padT = 4.f, padB = 4.f;
    const float rH = cH - padT - padB;

    // Helper: note Y position
    auto noteY = [&](int note) -> float {
        return content.getY() + padT + (1.f - (float)(note - minNote) / (float)noteRange) * rH;
    };
    float rowH = juce::jmax (3.f, rH / (float)noteRange - 0.5f);

    // ── Note axis — left strip ────────────────────────────────────────
    // Background for axis
    g.setColour (juce::Colour (C_BG).withAlpha (0.7f));
    g.fillRect (fa.getX(), fa.getY(), labelW - 1.f, fa.getHeight());
    g.setColour (juce::Colour (C_DIM).withAlpha (0.2f));
    g.drawVerticalLine ((int)(fa.getX() + labelW - 1), fa.getY(), fa.getBottom());

    // Draw note names and horizontal guide lines for every note in range
    g.setFont ({ 8.f });
    for (int note = minNote; note <= maxNote; ++note)
    {
        float y = noteY (note);
        int   pc = note % 12;
        bool  isC = (pc == 0);

        // Guide line across content area
        if (isC || pc == 5)   // C and F lines are brightest
        {
            g.setColour (juce::Colour (isC ? C_DIM : C_PANEL).withAlpha (isC ? 0.35f : 0.20f));
            g.drawHorizontalLine ((int)y, content.getX(), content.getRight());
        }

        // Draw label only for C, E, G, A (avoid crowding)
        if (pc == 0 || pc == 4 || pc == 7 || pc == 9)
        {
            juce::Colour labelCol = isC ? juce::Colour (C_GREEN)
                                        : juce::Colour (C_DIM).withAlpha (0.7f);
            g.setColour (labelCol);
            juce::String label = juce::String (NOTE_NAMES[pc])
                               + juce::String (note / 12 - 1);
            g.drawText (label,
                        (int)fa.getX(), (int)(y - 5.f),
                        (int)labelW - 3, 10,
                        juce::Justification::centredRight);
        }

        // Black key background stripe
        bool isBlack = (pc==1||pc==3||pc==6||pc==8||pc==10);
        if (isBlack)
        {
            g.setColour (juce::Colours::black.withAlpha (0.12f));
            g.fillRect (content.getX(), y, cW, rowH);
        }
    }

    // ── MIDI note blocks ─────────────────────────────────────────────
    for (auto& ev : events)
    {
        if (ev.note < minNote || ev.note > maxNote) continue;

        double dur = ev.duration > 0.0 ? ev.duration : 0.08;
        float  x   = content.getX() + (float)((ev.startTime - startTime) / windowSec) * cW;
        float  w   = juce::jmax (2.f, (float)(dur / windowSec) * cW);
        float  y   = noteY (ev.note);

        bool isVoice = ev.channel == 1;
        bool isHarm  = ev.channel == 2;
        juce::Colour nc = isVoice ? juce::Colour (C_AMBER)
                        : isHarm  ? juce::Colour (C_CYAN)
                                  : juce::Colour (C_GREEN);

        // Block body
        g.setColour (nc.withAlpha (0.80f));
        g.fillRoundedRectangle (x, y, w, rowH, 1.5f);

        // Bright top edge highlight
        g.setColour (nc.brighter (0.4f).withAlpha (0.6f));
        g.fillRect (x, y, w, 1.5f);

        // Note label inside block (if wide enough)
        if (w > 18.f)
        {
            g.setColour (juce::Colour (C_BG).withAlpha (0.85f));
            g.setFont ({ 7.5f, juce::Font::bold });
            juce::String noteLabel = juce::String (NOTE_NAMES[ev.note % 12])
                                   + juce::String (ev.note / 12 - 1);
            g.drawText (noteLabel, (int)(x + 2.f), (int)y, (int)(w - 3.f), (int)rowH,
                        juce::Justification::centredLeft);
        }
    }

    // ── Legend ───────────────────────────────────────────────────────
    const float legX = content.getRight() - 140.f;
    const float legY = content.getY() + 4.f;
    g.setFont ({ 8.f });
    struct { juce::uint32 col; const char* label; } legend[] = {
        { C_GREEN, "Chord/Virtual" },
        { C_AMBER, "Voice"         },
        { C_CYAN,  "Harmony"       },
    };
    float lx = legX;
    for (auto& l : legend)
    {
        g.setColour (juce::Colour (l.col).withAlpha (0.85f));
        g.fillRoundedRectangle (lx, legY, 8.f, 8.f, 2.f);
        g.setColour (juce::Colour (C_DIM));
        g.drawText (l.label, (int)(lx + 10.f), (int)(legY - 1.f), 70, 10,
                    juce::Justification::centredLeft);
        lx += 82.f;
    }
}

//==============================================================================
void ChordDetectEditor::paintPianoKeys (juce::Graphics& g, juce::Rectangle<int> area)
{
    // Reserve top strip for octave buttons + label
    auto topStrip = area.toFloat().removeFromTop (26.f);
    auto fa = area.toFloat().reduced (10.f, 4.f).withTrimmedTop (22.f);

    const int numOctaves = 2;
    const int tot = numOctaves * 7;
    float wkW = fa.getWidth() / (float)tot;
    float wkH = fa.getHeight();
    float bkW = wkW * 0.58f;
    float bkH = wkH * 0.60f;

    g.setColour (juce::Colour (C_PANEL));
    g.fillRoundedRectangle (fa.expanded (2.f), 4.f);

    // Octave range label
    int hiOct = keyboardBaseNote + 24;
    juce::String octLabel = juce::String (NOTE_NAMES[keyboardBaseNote % 12])
                          + juce::String (keyboardBaseNote / 12 - 1)
                          + " - "
                          + juce::String (NOTE_NAMES[(hiOct - 1) % 12])
                          + juce::String ((hiOct - 1) / 12 - 1)
                          + "  (click to play)";
    g.setColour (juce::Colour (C_DIM));
    g.setFont ({ 9.5f });
    g.drawText (octLabel, (int)topStrip.getX() + 106, (int)topStrip.getY(),
                (int)topStrip.getWidth() - 110, (int)topStrip.getHeight(),
                juce::Justification::centredLeft);

    // White keys
    for (int oct = 0; oct < numOctaves; ++oct)
        for (int k = 0; k < 7; ++k)
        {
            int  midiNote = keyboardBaseNote + oct * 12 + WHITE_KEYS[k];
            int  pc       = WHITE_KEYS[k];
            bool chord    = lastNotes.count (pc) > 0;
            bool voice    = lastVoiceNote >= 0 && lastVoiceNote % 12 == pc;
            bool virt     = proc.keyboardState.isNoteOnForChannels (0xffff, midiNote);
            float x = fa.getX() + (oct * 7 + k) * wkW;
            juce::Rectangle<float> r (x, fa.getY(), wkW - 1.5f, wkH);

            if (virt)
            {
                juce::ColourGradient gr (juce::Colour(C_WHITE), r.getCentreX(), r.getY(),
                                          juce::Colour(C_AMBER).withAlpha(0.7f), r.getCentreX(), r.getBottom(), false);
                g.setGradientFill (gr);
            }
            else if (voice)  g.setColour (juce::Colour (C_AMBER));
            else if (chord)
            {
                juce::ColourGradient gr (juce::Colour(C_GREEN), r.getCentreX(), r.getY(),
                                          juce::Colour(C_GREEN).darker(0.4f), r.getCentreX(), r.getBottom(), false);
                g.setGradientFill (gr);
            }
            else g.setColour (juce::Colour (C_WHITE));

            g.fillRoundedRectangle (r, 2.5f);
            g.setColour (juce::Colour (C_PANEL));
            g.drawRoundedRectangle (r, 2.5f, 1.f);

            // Note name on C keys
            if (WHITE_KEYS[k] == 0)
            {
                g.setColour (juce::Colour (C_DIM));
                g.setFont ({ 8.f });
                g.drawText (juce::String (NOTE_NAMES[0]) + juce::String (midiNote / 12 - 1),
                            (int)r.getX(), (int)r.getBottom() - 12, (int)wkW, 12,
                            juce::Justification::centred);
            }
        }

    // Black keys
    for (int oct = 0; oct < numOctaves; ++oct)
        for (int k = 0; k < 7; ++k)
        {
            if (BLACK_KEYS[k] < 0) continue;
            int  midiNote = keyboardBaseNote + oct * 12 + BLACK_KEYS[k];
            int  pc       = BLACK_KEYS[k];
            bool chord    = lastNotes.count (pc) > 0;
            bool voice    = lastVoiceNote >= 0 && lastVoiceNote % 12 == pc;
            bool virt     = proc.keyboardState.isNoteOnForChannels (0xffff, midiNote);
            float cx2 = fa.getX() + (oct * 7 + k + 1) * wkW;
            juce::Rectangle<float> r (cx2 - bkW * 0.5f, fa.getY(), bkW, bkH);

            if (virt)        g.setColour (juce::Colour (C_AMBER).darker (0.1f));
            else if (voice)  g.setColour (juce::Colour (C_AMBER).darker (0.2f));
            else if (chord)  g.setColour (juce::Colour (C_GREEN).darker (0.15f));
            else
            {
                juce::ColourGradient gr (juce::Colour (0xFF1A2E1A), r.getCentreX(), r.getY(),
                                          juce::Colour (C_BG), r.getCentreX(), r.getBottom(), false);
                g.setGradientFill (gr);
            }
            g.fillRoundedRectangle (r, 2.5f);
            g.setColour (juce::Colour (C_BG).withAlpha (0.6f));
            g.drawRoundedRectangle (r, 2.5f, 0.5f);
        }
}

//==============================================================================
// Skull mic + Johnny Nix logo (unchanged from previous)
//==============================================================================
void ChordDetectEditor::paintSkullMic (juce::Graphics& g, juce::Rectangle<float> a, bool active)
{
    float cx=a.getCentreX(), cy=a.getCentreY(), s=a.getHeight()/100.f;
    juce::Colour col=active?juce::Colour(C_GREEN):juce::Colour(C_RED), glow=col.withAlpha(0.18f), bg=juce::Colour(C_BG);
    float hx=cx-8*s,hy=cy+16*s,hw=16*s,hh=24*s;
    g.setColour(col.withAlpha(0.75f)); g.fillRoundedRectangle(hx,hy,hw,hh,7*s);
    g.setColour(bg.withAlpha(0.45f)); for(int i=1;i<=5;++i) g.fillRect(hx+2*s,hy+i*(hh/6.f),hw-4*s,1.f);
    g.setColour(col.withAlpha(0.55f)); g.fillRoundedRectangle(cx-14*s,hy+hh+s,28*s,5*s,2.5f*s);
    float ht=cy-40*s,hw2=38*s,hh2=36*s;
    juce::Path halo; halo.addEllipse(cx-hw2*.65f,ht-hh2*.1f,hw2*1.3f,hh2*1.2f);
    g.setColour(glow); g.fillPath(halo);
    juce::Path cr; cr.addEllipse(cx-hw2*.5f,ht,hw2,hh2);
    g.setColour(bg); g.fillPath(cr); g.setColour(col); g.strokePath(cr, juce::PathStrokeType(1.8f));
    float jt=ht+hh2*.62f,jw=hw2*.76f,jh=hh2*.46f;
    juce::Path jaw; jaw.addRoundedRectangle(cx-jw*.5f,jt,jw,jh,jw*.18f);
    g.setColour(bg); g.fillPath(jaw); g.setColour(col); g.strokePath(jaw, juce::PathStrokeType(1.5f));
    float ew=hw2*.22f,eh=hh2*.28f,ey=ht+hh2*.18f;
    for(int side:{-1,1}){float ex=cx+side*hw2*.22f-ew*.5f; g.setColour(glow); g.fillEllipse(ex,ey,ew,eh); g.setColour(col); g.drawEllipse(ex,ey,ew,eh,1.5f); g.fillEllipse(ex+ew*.3f,ey+eh*.3f,ew*.35f,eh*.35f);}
    float ny=ht+hh2*.52f; juce::Path nose; nose.startNewSubPath(cx,ny-4.5f*s); nose.lineTo(cx-3.5f*s,ny+s); nose.lineTo(cx-1.5f*s,ny+s); nose.startNewSubPath(cx,ny-4.5f*s); nose.lineTo(cx+3.5f*s,ny+s); nose.lineTo(cx+1.5f*s,ny+s); g.setColour(col); g.strokePath(nose, juce::PathStrokeType(1.2f));
    for(int i=-1;i<=2;++i){float tx=cx-jw*.5f+jw*.1f+i*jw*.215f; g.fillRoundedRectangle(tx,jt+s,jw*.17f,jh*.46f,1.5f);}
    juce::Path crack; crack.startNewSubPath(cx+2*s,ht+hh2*.08f); crack.lineTo(cx-3*s,ht+hh2*.22f); crack.lineTo(cx+s,ht+hh2*.34f); g.setColour(col.withAlpha(0.5f)); g.strokePath(crack, juce::PathStrokeType(1.f));
    juce::Path cable; cable.startNewSubPath(cx,hy+hh+6*s); cable.quadraticTo(cx+10*s,hy+hh+15*s,cx+5*s,hy+hh+22*s); g.setColour(col.withAlpha(0.4f)); g.strokePath(cable, juce::PathStrokeType(1.2f));
}

void ChordDetectEditor::paintCowboyHat (juce::Graphics& g, float cx, float topY, float w, juce::Colour col)
{
    float cW=w*.9f,cH=w*.82f,bW=w*1.8f,bH=w*.22f,bY=topY+cH,dip=cH*.14f;
    juce::Path crown;
    crown.startNewSubPath(cx-cW*.5f,bY);
    crown.cubicTo(cx-cW*.58f,bY-cH*.35f,cx-cW*.46f,topY+dip,cx-cW*.14f,topY+dip);
    crown.cubicTo(cx-cW*.04f,topY,cx+cW*.04f,topY,cx+cW*.14f,topY+dip);
    crown.cubicTo(cx+cW*.46f,topY+dip,cx+cW*.58f,bY-cH*.35f,cx+cW*.5f,bY);
    crown.closeSubPath();
    g.setColour(col); g.fillPath(crown); g.setColour(col.darker(0.35f)); g.strokePath(crown, juce::PathStrokeType(0.7f));
    g.setColour(col.darker(0.5f)); g.fillRect(cx-cW*.5f,bY-cH*.20f,cW,cH*.10f);
    juce::Path brim;
    brim.startNewSubPath(cx-bW*.5f,bY+bH*.25f);
    brim.cubicTo(cx-bW*.38f,bY-bH*.6f,cx-cW*.5f,bY-bH*.2f,cx-cW*.5f,bY);
    brim.lineTo(cx+cW*.5f,bY);
    brim.cubicTo(cx+cW*.5f,bY-bH*.2f,cx+bW*.38f,bY-bH*.6f,cx+bW*.5f,bY+bH*.25f);
    brim.cubicTo(cx+bW*.38f,bY+bH*1.1f,cx-bW*.38f,bY+bH*1.1f,cx-bW*.5f,bY+bH*.25f);
    brim.closeSubPath();
    g.setColour(col); g.fillPath(brim); g.setColour(col.darker(0.35f)); g.strokePath(brim, juce::PathStrokeType(0.7f));
}

void ChordDetectEditor::paintJohnnyNixLogo (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour col)
{
    juce::Font font("Helvetica Neue", 15.f, juce::Font::bold);
    const float baseY = area.getCentreY() + font.getAscent()*0.36f;
    const float startX = area.getX() + 4.f;
    juce::GlyphArrangement ga;
    ga.addLineOfText(font, "JOHNNY NIX", startX, baseY);
    juce::Path textPath;
    for(int i=0;i<ga.getNumGlyphs();++i) if(i!=1) ga.getGlyph(i).createPath(textPath);
    g.setColour(col); g.fillPath(textPath);
    if(ga.getNumGlyphs()>1) {
        juce::Path oPath; ga.getGlyph(1).createPath(oPath); g.fillPath(oPath);
        const auto& og=ga.getGlyph(1);
        float oCX=(og.getLeft()+og.getRight())*.5f, oW=og.getRight()-og.getLeft();
        float oTop=baseY-font.getAscent()*.92f;
        paintCowboyHat(g,oCX,oTop-oW*.95f,oW,col);
    }
}
