# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Chordy is a JUCE C++17 audio plugin for chord voicing practice with spaced repetition. It receives MIDI input, displays a keyboard visualization, identifies chords, and lets users record voicings into a library and practice them across all 12 keys. Targets VST3, AU, and Standalone on macOS. Company: "JezzterInc".

## Build Commands

```bash
make build    # cmake -B cmake-build && cmake --build cmake-build
make run      # build + open Standalone app
```

Build-specific format:
```bash
cmake --build cmake-build --target Chordy_VST3
cmake --build cmake-build --target Chordy_AU
cmake --build cmake-build --target Chordy_Standalone
```

Artifacts: `cmake-build/Chordy_artefacts/Standalone/Chordy.app` (and VST3/AU equivalents).

`COPY_PLUGIN_AFTER_BUILD=TRUE` auto-installs built plugins to system directories.

## JUCE Dependency

JUCE is included as a **git submodule** at `./JUCE/` via `add_subdirectory(JUCE)`.

## Architecture

### Source Files (all in `Source/`)

| File | Purpose |
|---|---|
| `PluginProcessor.h/.cpp` | Audio/MIDI processing hub. Owns `MidiKeyboardState`, APVTS, `VoicingLibrary`, `SpacedRepetitionEngine`. Lock-free note sharing via atomic bitfield. |
| `PluginEditor.h/.cpp` | Top-level GUI. Hosts keyboard, chord label, voicing library panel, practice panel. 60Hz timer drives chord detection and practice updates. |
| `ChordDetector.h/.cpp` | Pure-logic chord identification. Pitch-class template matching against known chord types (triads through 9ths). |
| `ChordyKeyboardComponent.h/.cpp` | `MidiKeyboardComponent` subclass with colored key overlays (green=correct, red=wrong, blue=target). |
| `VoicingModel.h/.cpp` | `Voicing` struct (intervals from root) + `VoicingLibrary` class with ValueTree serialization. |
| `SpacedRepetition.h/.cpp` | SM-2 spaced repetition engine. Tracks per-voicing per-key practice records. |
| `VoicingLibraryPanel.h/.cpp` | GUI panel for recording, viewing, filtering, and deleting voicings. |
| `PracticePanel.h/.cpp` | GUI panel for spaced repetition practice with visual feedback. |

### MIDI Data Flow

```
processBlock() [AUDIO THREAD]
  └─ keyboardState.processNextMidiBuffer() → bridges to GUI
  └─ Scan notes → write to std::atomic<uint64_t>[2] bitfield

timerCallback() [GUI THREAD, 60Hz]
  └─ Read atomic bitfield → ChordDetector::detect() → update chord label
  └─ If practicing: compare notes → color keyboard → update SR state
```

### Data Model

- Voicings stored as **intervals from root** (e.g. `[0, 4, 7, 11]` = maj7) — inherently transposable
- `ChordQuality` enum covers triads, 7ths, 9ths, sus, aug, dim variants
- All structured data serialized via `ValueTree` (appended to APVTS state)

## Adding Source Files

All new `.cpp`/`.h` files **must** be added to the `SOURCE_FILES` variable in `CMakeLists.txt`.

## Plugin Configuration (CMakeLists.txt)

- MIDI Input: enabled | MIDI Output: enabled | IS_SYNTH: false
- Formats: VST3, AU, Standalone
- Manufacturer code: `Tap1` | Plugin code: `Chrd`
- `EDITOR_WANTS_KEYBOARD_FOCUS: TRUE` (for on-screen keyboard input)

## Key Development Notes

- Audio thread rules: no allocations, no blocking, no locks in `processBlock()`
- `MidiKeyboardState` bridges audio↔GUI threads safely
- Active notes shared via `std::atomic<uint64_t>` bitfield (lock-free)
- No test infrastructure exists yet
