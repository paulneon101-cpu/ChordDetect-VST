# ChordDetect VST

A real-time chord detection VST3 audio plugin built with JUCE. Feed it any audio — guitar, piano, synth — and it identifies the chord playing, displaying the root note, quality, and individual pitch classes on an interactive piano keyboard.

## Features

- Real-time FFT-based pitch detection (2048-point with Hann window)
- Detects 25+ chord types: triads, sevenths, ninths, elevenths, thirteenths, sus, aug, dim, power chords
- Pitch-class normalization (octave-independent detection)
- Animated piano keyboard showing active notes
- Dark UI with accent-lit chord display
- VST3 + Standalone formats
- MIDI output of chord root

## Building

### Prerequisites

- [CMake](https://cmake.org/) 3.22+
- [Git](https://git-scm.com/)
- A C++17 compiler:
  - **Windows**: Visual Studio 2022 (with "Desktop development with C++")
  - **macOS**: Xcode 13+
  - **Linux**: GCC 10+ or Clang 12+

JUCE is fetched automatically via CMake's `FetchContent` — no manual download needed.

### Steps

```bash
git clone https://github.com/YOUR_USERNAME/ChordDetect-VST.git
cd ChordDetect-VST
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The built `.vst3` bundle is in `build/ChordDetect_artefacts/Release/VST3/`.

### Windows (Visual Studio)

```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

### macOS (Xcode)

```bash
cmake -B build -G Xcode
cmake --build build --config Release
```

## How It Works

1. **PitchDetector** — buffers incoming audio into 2048-sample frames, applies a Hann window, runs an FFT, normalizes the spectrum, and picks local magnitude peaks above a threshold. Each peak frequency is converted to a MIDI note number and stored as a pitch class (0–11).

2. **ChordDetector** — takes the set of active pitch classes and scores every combination of root (12 notes) × chord template (25 types). The template that best explains the input — maximizing covered notes, minimizing unexplained notes — wins.

3. **PluginEditor** — polls the processor at 20 Hz and repaints the chord name and piano keyboard when the detected chord changes.

## Chord Types Detected

| Symbol | Name |
|--------|------|
| (none) | Major |
| min | Minor |
| 7 | Dominant 7th |
| maj7 | Major 7th |
| min7 | Minor 7th |
| dim | Diminished |
| dim7 | Diminished 7th |
| aug | Augmented |
| sus2 | Suspended 2nd |
| sus4 | Suspended 4th |
| m7b5 | Half-diminished |
| 6 / min6 | Sixth |
| maj9 / 9 / min9 | Ninth |
| add9 | Add 9 |
| 11 / maj11 | Eleventh |
| 13 / maj13 | Thirteenth |
| 5 | Power chord |

## License

MIT
