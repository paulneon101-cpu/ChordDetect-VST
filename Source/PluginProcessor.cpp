#include "PluginProcessor.h"
#include "PluginEditor.h"

ChordDetectProcessor::ChordDetectProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

ChordDetectProcessor::~ChordDetectProcessor() {}

//==============================================================================
void ChordDetectProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    pitchDetector.prepare (sampleRate, samplesPerBlock);
}

void ChordDetectProcessor::releaseResources() {}

void ChordDetectProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals noDenormals;

    // Pass audio through unchanged
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
void ChordDetectProcessor::getStateInformation (juce::MemoryBlock& /*destData*/) {}
void ChordDetectProcessor::setStateInformation (const void* /*data*/, int /*sz*/) {}

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
