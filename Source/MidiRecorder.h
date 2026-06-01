#pragma once
#include <JuceHeader.h>
#include <vector>
#include <map>

class MidiRecorder
{
public:
    MidiRecorder() = default;
    struct NoteEvent
    {
        int    note;
        int    channel;
        int    velocity;
        double startTime;   // seconds from recorder start
        double duration;    // seconds (-1 while note is still held)
    };

    void   prepare     (double sampleRate);
    void   record      (const juce::MidiBuffer& midi, int numSamples);

    void   setRecording (bool r);
    bool   isRecording  () const { return recording; }
    void   clear        ();
    void   transpose    (int semitones);
    void   quantize     (double bpm = 120.0, int division = 16);   // snap to 1/division

    std::vector<NoteEvent> getEvents() const;
    double                 getDuration() const;

private:
    bool   recording  { false };
    double sampleRate { 44100.0 };
    double currentTime { 0.0 };

    static constexpr double MAX_RECORD_TIME = 30.0;   // 30 s rolling window

    mutable juce::CriticalSection lock;
    std::vector<NoteEvent>     events;
    std::map<int, double>      activeNotes;  // note -> startTime

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiRecorder)
};
