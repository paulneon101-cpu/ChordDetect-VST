#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
ChordDetectProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto addFloat = [&](const char* id, const char* name,
                        float lo, float hi, float def, const char* label = "")
    {
        p.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID{id,1}, name,
            juce::NormalisableRange<float>(lo,hi,0.01f), def,
            juce::AudioParameterFloatAttributes{}.withLabel(label)));
    };
    auto addChoice = [&](const char* id, const char* name,
                         juce::StringArray choices, int def)
    {
        p.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID{id,1}, name, choices, def));
    };
    auto addBool = [&](const char* id, const char* name, bool def)
    {
        p.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID{id,1}, name, def));
    };

    addFloat  (PID_PITCH,      "Pitch Correction",  0.f, 100.f, 50.f, "%");
    addChoice (PID_LATENCY,    "Latency Mode",       {"Low","Balanced","High"}, 1);
    addChoice (PID_HARMONY,    "Harmony Mode",       {"Off","+3rd","+5th","+Oct","Chord"}, 0);
    addBool   (PID_VOICE_ON,   "Voice to MIDI",      false);
    addFloat  (PID_VOICE_SENS, "Voice Sensitivity",  0.f, 1.f, 0.5f);
    addFloat  (PID_EQ_LOW,     "Bass Gain",          -12.f, 12.f,  0.f, "dB");
    addFloat  (PID_EQ_LOW_HZ,  "Bass Frequency",      20.f,500.f, 80.f, "Hz");
    addFloat  (PID_EQ_MID,     "Mid Gain",           -12.f, 12.f,  0.f, "dB");
    addFloat  (PID_EQ_MID_HZ,  "Mid Frequency",      200.f,5000.f,1000.f,"Hz");
    addFloat  (PID_EQ_HIGH,    "High Gain",          -12.f, 12.f,  0.f, "dB");
    addFloat  (PID_EQ_HIGH_HZ, "High Frequency",    2000.f,20000.f,8000.f,"Hz");
    addFloat  (PID_REV_ROOM,   "Reverb Room",        0.f, 1.f, 0.3f);
    addFloat  (PID_REV_WET,    "Reverb Wet",         0.f, 1.f, 0.f);
    addFloat  (PID_DLY_TIME,   "Delay Time",         1.f,1000.f,250.f,"ms");
    addFloat  (PID_DLY_FDBK,   "Delay Feedback",     0.f, 0.92f,0.3f);
    addFloat  (PID_DLY_WET,    "Delay Wet",          0.f, 1.f, 0.f);
    addBool   (PID_MIDI_REC,   "MIDI Record",        false);
    addBool   (PID_MIC_HIT,   "Mic Hit",            false);
    addFloat  (PID_MIC_HIT_SENS, "Mic Hit Sens",    0.f, 1.f, 0.5f);

    return { p.begin(), p.end() };
}

//==============================================================================
ChordDetectProcessor::ChordDetectProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}
ChordDetectProcessor::~ChordDetectProcessor() {}

//==============================================================================
void ChordDetectProcessor::prepareToPlay (double sr, int blockSize)
{
    currentSampleRate = sr;
    currentBlockSize  = blockSize;

    int latMode = (int)apvts.getRawParameterValue(PID_LATENCY)->load();
    lastLatencyMode = latMode;
    pitchDetector.setFFTOrder (9 + latMode);
    pitchDetector.prepare (sr, blockSize);

    monoPitchDetector.prepare (sr, blockSize);
    midiRecorder     .prepare (sr);

    fxChain.prepare (sr, blockSize,
        getTotalNumInputChannels() > 0 ? getTotalNumInputChannels() : 2);

    lastVoiceMidiNote = -1;
}

void ChordDetectProcessor::releaseResources() {}

//==============================================================================
void ChordDetectProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    // ── Virtual keyboard + hardware MIDI pass-through ─────────────────
    // keyboardState injects virtual piano clicks; hardware MIDI is already in buffer
    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);

    // ── Read parameters ───────────────────────────────────────────────
    pitchDetector.setPitchCorrection (apvts.getRawParameterValue(PID_PITCH)->load());

    const int latMode = (int)apvts.getRawParameterValue(PID_LATENCY)->load();
    if (latMode != lastLatencyMode)
    {
        lastLatencyMode = latMode;
        pitchDetector.setFFTOrder (9 + latMode);
        pitchDetector.prepare (currentSampleRate, currentBlockSize);
    }

    const bool  voiceOn  = apvts.getRawParameterValue(PID_VOICE_ON)  ->load() > 0.5f;
    const float voiceSns = apvts.getRawParameterValue(PID_VOICE_SENS)->load();
    monoPitchDetector.setEnabled     (voiceOn);
    monoPitchDetector.setSensitivity (voiceSns);

    harmonyGenerator.setMode (static_cast<HarmonyGenerator::Mode> (
        (int)apvts.getRawParameterValue(PID_HARMONY)->load()));

    fxChain.setEQ     (apvts.getRawParameterValue(PID_EQ_LOW)    ->load(),
                       apvts.getRawParameterValue(PID_EQ_LOW_HZ) ->load(),
                       apvts.getRawParameterValue(PID_EQ_MID)    ->load(),
                       apvts.getRawParameterValue(PID_EQ_MID_HZ) ->load(),
                       apvts.getRawParameterValue(PID_EQ_HIGH)   ->load(),
                       apvts.getRawParameterValue(PID_EQ_HIGH_HZ)->load());
    fxChain.setReverb (apvts.getRawParameterValue(PID_REV_ROOM)->load(),
                       apvts.getRawParameterValue(PID_REV_WET) ->load());
    fxChain.setDelay  (apvts.getRawParameterValue(PID_DLY_TIME)->load(),
                       apvts.getRawParameterValue(PID_DLY_FDBK)->load(),
                       apvts.getRawParameterValue(PID_DLY_WET) ->load());

    const bool midiRec = apvts.getRawParameterValue(PID_MIDI_REC)->load() > 0.5f;
    midiRecorder.setRecording (midiRec);

    // ── Analysis ──────────────────────────────────────────────────────
    pitchDetector    .processBlock (buffer);
    monoPitchDetector.processBlock (buffer);

    const auto notes     = pitchDetector.getActiveNotes();
    const auto chord     = chordDetector.detect (notes);
    const int  voiceNote = monoPitchDetector.getCurrentMidiNote();

    // ── Voice-to-MIDI ─────────────────────────────────────────────────
    if (voiceOn)
    {
        if (voiceNote != lastVoiceMidiNote)
        {
            if (lastVoiceMidiNote >= 0)
                midi.addEvent (juce::MidiMessage::noteOff (VOICE_MIDI_CH, lastVoiceMidiNote, (uint8_t)0), 0);
            if (voiceNote >= 0)
                midi.addEvent (juce::MidiMessage::noteOn  (VOICE_MIDI_CH, voiceNote, (uint8_t)100), 0);
            lastVoiceMidiNote = voiceNote;
        }
    }
    else if (lastVoiceMidiNote >= 0)
    {
        midi.addEvent (juce::MidiMessage::noteOff (VOICE_MIDI_CH, lastVoiceMidiNote, (uint8_t)0), 0);
        lastVoiceMidiNote = -1;
    }

    // ── Mic Hit (noise-to-MIDI trigger) ──────────────────────────────
    const bool  micHitOn   = apvts.getRawParameterValue(PID_MIC_HIT)     ->load() > 0.5f;
    const float micHitSens = apvts.getRawParameterValue(PID_MIC_HIT_SENS)->load();
    if (micHitOn)
    {
        float rms = buffer.getRMSLevel (0, 0, buffer.getNumSamples());
        micHitEnvelope = (rms > micHitEnvelope)
                       ? rms * 0.30f + micHitEnvelope * 0.70f
                       : rms * 0.05f + micHitEnvelope * 0.95f;

        const float onThresh  = 0.08f - micHitSens * 0.075f;
        const float offThresh = onThresh * 0.40f;

        if (!micHitActive && micHitEnvelope > onThresh)
        {
            int trigNote = (voiceNote >= 0) ? voiceNote
                         : (chord.isValid   ? 60 + chord.rootNote : 60);
            trigNote = juce::jlimit (0, 127, trigNote);
            auto vel = (juce::uint8)juce::jlimit (1, 127, (int)(micHitEnvelope * 800.f));
            midi.addEvent (juce::MidiMessage::noteOn  (MIC_HIT_MIDI_CH, trigNote, vel), 0);
            micHitNote   = trigNote;
            micHitActive = true;
        }
        else if (micHitActive && micHitEnvelope < offThresh)
        {
            midi.addEvent (juce::MidiMessage::noteOff (MIC_HIT_MIDI_CH, micHitNote, (juce::uint8)0), 0);
            micHitActive = false;
        }
    }
    else if (micHitActive)
    {
        midi.addEvent (juce::MidiMessage::noteOff (MIC_HIT_MIDI_CH, micHitNote, (juce::uint8)0), 0);
        micHitActive = false;
    }

    // ── Harmony ───────────────────────────────────────────────────────
    if (harmonyGenerator.update (chord, notes, voiceOn ? voiceNote : -1))
        harmonyGenerator.fillMidi (midi, 0, HARMONY_MIDI_CH);

    // ── Suggestion playback ───────────────────────────────────────────
    {
        juce::ScopedLock sl (suggLock);
        if (!pendingSuggNotes.empty())
        {
            for (int n : lastSuggNotes)
                midi.addEvent (juce::MidiMessage::noteOff (3, n, (uint8_t)0), 0);
            lastSuggNotes.clear();
            for (int n : pendingSuggNotes)
            {
                midi.addEvent (juce::MidiMessage::noteOn (3, n, (uint8_t)90), 0);
                lastSuggNotes.push_back (n);
            }
            pendingSuggNotes.clear();
        }
    }

    // ── FX chain ─────────────────────────────────────────────────────
    fxChain.processBlock (buffer);

    // ── MIDI record ───────────────────────────────────────────────────
    midiRecorder.record (midi, buffer.getNumSamples());

    // ── Share state ───────────────────────────────────────────────────
    {
        juce::ScopedLock sl (dataLock);
        currentNotes      = notes;
        currentChord      = chord;
        currentVoiceNote  = voiceNote;
        currentSuggestions = chordAdvisor.suggest (chord);
    }
}

//==============================================================================
juce::AudioProcessorEditor* ChordDetectProcessor::createEditor()
    { return new ChordDetectEditor (*this); }

void ChordDetectProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, dest);
}
void ChordDetectProcessor::setStateInformation (const void* data, int sz)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sz));
    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

ChordDetector::ChordInfo ChordDetectProcessor::getCurrentChord() const
    { juce::ScopedLock sl(dataLock); return currentChord; }
std::set<int> ChordDetectProcessor::getCurrentNotes() const
    { juce::ScopedLock sl(dataLock); return currentNotes; }
int ChordDetectProcessor::getCurrentVoiceNote() const
    { juce::ScopedLock sl(dataLock); return currentVoiceNote; }
std::vector<MidiRecorder::NoteEvent> ChordDetectProcessor::getRecordedEvents() const
    { return midiRecorder.getEvents(); }
double ChordDetectProcessor::getRecorderTime() const
    { return midiRecorder.getDuration(); }
std::vector<ChordAdvisor::Suggestion> ChordDetectProcessor::getSuggestions() const
    { juce::ScopedLock sl(dataLock); return currentSuggestions; }

void ChordDetectProcessor::clearRecording()     { midiRecorder.clear(); }
void ChordDetectProcessor::transposeRecording(int s) { midiRecorder.transpose(s); }
void ChordDetectProcessor::quantizeRecording()  { midiRecorder.quantize(); }

void ChordDetectProcessor::playSuggestion (int idx, juce::MidiBuffer& /*dest*/)
{
    auto suggs = getSuggestions();
    if (idx < 0 || idx >= (int)suggs.size()) return;
    juce::ScopedLock sl (suggLock);
    pendingSuggNotes = suggs[(size_t)idx].midiNotes;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
    { return new ChordDetectProcessor(); }
