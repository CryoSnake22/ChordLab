# Recording and Practice

## Recording

All recording (voicings, progressions, melodies) captures:
- **Velocity**: Per-note velocity stored and preserved during playback/preview.
- **Sustain pedal**: CC64 events recorded and replayed.
- **Raw MIDI**: Original performance preserved. All quantize levels re-analyze from raw MIDI (non-destructive -- switching quantize never destroys detail).
- **Quantize modes**: Raw (exact timing), Beat (1.0), 1/2 (0.5), 1/4 (0.25). Raw is the default on recording stop.
- **totalBeats** rounds up to nearest 4-beat bar to preserve trailing silence.

### Two-Pass Velocity Capture

Per-note velocities tracked via `noteVelocities[128]` atomic array in processor. Two passes in processBlock: first before `processNextMidiBuffer` catches external MIDI, second after catches on-screen/QWERTY keyboard events. Without the second pass, on-screen keyboard voicing recordings get zero velocities.

## Practice Modes

### Order Modes
- **Chromatic** (default): Up-and-back through all 12 keys (C, C#, ..., B, Bb, ..., C#).
- **Random**: Random key selection.
- **Follow** (voicing only): Root walks along a chosen scale or melody's pitch classes. Pure transposition.
- **Scale/Diatonic** (voicing only): Each note moves independently by scale degrees via `diatonicTranspose()`. Only available when `voicingFitsInScale()` returns true.

Follow and Scale are disabled for progression/melody practice.

### Scoring Rules

- **Per-note independent scoring** -- each note is scored individually. Never chord-level. Chord grouping causes cascading false positives.
- **Progression practice**: Each note scored by whether user plays its pitch class during `[noteStartBeat, noteStartBeat+noteDuration)` window. Notes: Target (amber) when active, Correct (green) when played, Missed (red) when expired.
- **Melody practice**: Per-note pitch-class matching with 0.5-beat coyote time for early hits. `scored` flag prevents double-scoring repeated notes.
- **Proportional scoring on skip/timeout**: Missing half the notes = ~50% quality. `computeProportionalMatch()` computes correct/total minus 0.1 per extra note. Maps to SM-2 quality 0-5 via `proportionToQuality()`.
- **`getNextChallenge`** accepts `avoidKey` parameter to prevent repeating the same root consecutively.
- **Library locked during practice**: Record/Play/Edit/Delete buttons, ListBox, search, folder combo, "..." button, and stats chart mouse interaction all disabled.

### Spaced Repetition

SM-2 algorithm with recency-weighted accuracy: `attemptHistory` vector (capped at 100). Most recent 10 attempts get 2x weight. Backward compatible with legacy successes/failures counts.

## Melody-Specific

- **Backing pad**: Root-position stacked thirds from `getChordTones()`. Root placed in C3-F#3 range, intervals stacking upward.
- **Intervals relative to key root**: `intervalFromKeyRoot` stored per note. Transposition shifts `keyPitchClass`, intervals unchanged.
