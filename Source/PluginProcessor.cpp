#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
ChordDetectProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { PID_PITCH, 1 },
        "Pitch Correction",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        50.0f,
        juce::AudioParameterFloatAttributes{}.withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { PID_LATENCY, 1 },
        "Latency Mode",
        juce::StringArray { "Low Latency", "Balanced", "High Accuracy" },
        1));  // default: Balanced

    return { params.begin(), params.end() };
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
void ChordDetectProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize  = samplesPerBlock;

    int latencyMode = (int)apvts.getRawParameterValue (PID_LATENCY)->load();
    lastLatencyMode = latencyMode;
    pitchDetector.setFFTOrder (9 + latencyMode);
    pitchDetector.prepare (sampleRate, samplesPerBlock);
}

void ChordDetectProcessor::releaseResources() {}

void ChordDetectProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals noDenormals;

    // Sync pitch correction every block (cheap atomic read)
    const float pitchPct = apvts.getRawParameterValue (PID_PITCH)->load();
    pitchDetector.setPitchCorrection (pitchPct);

    // Reinitialise FFT only when latency mode actually changes
    const int latencyMode = (int)apvts.getRawParameterValue (PID_LATENCY)->load();
    if (latencyMode != lastLatencyMode)
    {
        lastLatencyMode = latencyMode;
        pitchDetector.setFFTOrder (9 + latencyMode);   // 9=512, 10=1024, 11=2048
        pitchDetector.prepare (currentSampleRate, currentBlockSize);
    }

    pitchDetector.processBlock (buffer);

    const auto notes = pitchDetector.getActiveNotes();
    const auto chord = chordDetector.detect (notes);

    {
        juce::ScopedLock sl (dataLock);
        currentNotes = notes;
        currentChord = chord;
    }
}

//==============================================================================
juce::AudioProcessorEditor* ChordDetectProcessor::createEditor()
{
    return new ChordDetectEditor (*this);
}

//==============================================================================
void ChordDetectProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ChordDetectProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
ChordDetector::ChordInfo ChordDetectProcessor::getCurrentChord() const
{
    juce::ScopedLock sl (dataLock);
    return currentChord;
}

std::set<int> ChordDetectProcessor::getCurrentNotes() const
{
    juce::ScopedLock sl (dataLock);
    return currentNotes;
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChordDetectProcessor();
}
