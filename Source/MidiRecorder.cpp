#include "MidiRecorder.h"

void MidiRecorder::prepare (double sr)
{
    sampleRate  = sr;
    currentTime = 0.0;
}

void MidiRecorder::setRecording (bool r)
{
    recording = r;
    if (!r)
    {
        // Close any open notes
        juce::ScopedLock sl (lock);
        for (auto& [note, startT] : activeNotes)
        {
            for (auto& ev : events)
                if (ev.note == note && ev.duration < 0.0)
                    ev.duration = currentTime - startT;
        }
        activeNotes.clear();
    }
}

void MidiRecorder::record (const juce::MidiBuffer& midi, int numSamples)
{
    const double blockDuration = numSamples / sampleRate;

    if (recording)
    {
        juce::ScopedLock sl (lock);

        for (const auto meta : midi)
        {
            const auto msg  = meta.getMessage();
            const double t  = currentTime + meta.samplePosition / sampleRate;

            if (msg.isNoteOn())
            {
                activeNotes[msg.getNoteNumber()] = t;
                events.push_back ({ msg.getNoteNumber(), msg.getChannel(),
                                    msg.getVelocity(), t, -1.0 });
            }
            else if (msg.isNoteOff())
            {
                int n = msg.getNoteNumber();
                if (activeNotes.count (n))
                {
                    double dur = t - activeNotes[n];
                    activeNotes.erase (n);
                    for (int i = (int)events.size() - 1; i >= 0; --i)
                        if (events[i].note == n && events[i].duration < 0.0)
                            { events[i].duration = dur; break; }
                }
            }
        }

        // Trim to rolling window
        currentTime += blockDuration;
        const double cutoff = currentTime - MAX_RECORD_TIME;
        events.erase (std::remove_if (events.begin(), events.end(),
            [cutoff](const NoteEvent& e){ return e.startTime < cutoff; }),
            events.end());
    }
    else
    {
        currentTime += blockDuration;
    }
}

void MidiRecorder::clear()
{
    juce::ScopedLock sl (lock);
    events.clear();
    activeNotes.clear();
}

void MidiRecorder::transpose (int semitones)
{
    juce::ScopedLock sl (lock);
    for (auto& ev : events)
        ev.note = juce::jlimit (0, 127, ev.note + semitones);
}

void MidiRecorder::quantize (double bpm, int division)
{
    juce::ScopedLock sl (lock);
    const double gridSec = 60.0 / bpm / (division / 4.0);
    for (auto& ev : events)
        ev.startTime = std::round (ev.startTime / gridSec) * gridSec;
}

std::vector<MidiRecorder::NoteEvent> MidiRecorder::getEvents() const
{
    juce::ScopedLock sl (lock);
    return events;
}

double MidiRecorder::getDuration() const
{
    juce::ScopedLock sl (lock);
    return currentTime;
}
