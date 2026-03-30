# External Instrument Hosting

VST3/AU plugin hosting for audible playback via external instruments. Works in both DAW and Standalone.

## Rules

- **Must use `createPluginInstanceAsync`** (not `Thread::launch` + synchronous `createPluginInstance`). Complex plugins like Keyscape need message thread access during initialization. Follow the JUCE `HostPluginDemo` pattern.
- **Must register formats manually** via `new VST3PluginFormat()` / `new AudioUnitPluginFormat()`. `addDefaultFormats()` is deleted in JUCE headless builds.
- **Bus layout negotiation is essential** before calling a hosted plugin's `processBlock`. AudioBuffer channel count must exactly match what the plugin expects. Use `setupPluginBuses()` after loading and a scratch buffer in `processBlock` for mismatches.
- **Thread safety**: `CriticalSection` (not SpinLock) protects hosted plugin pointer. Audio thread uses `ScopedTryLock` (non-blocking). All other paths use `ScopedLock`.
- **3x gain boost** applied to external instrument output to match internal synth levels.

## Crash Protection

- **Scan dead-mans-pedal** (`deadMansPedal.txt`): Blacklists plugins that crash during scanning.
- **Loading dead-mans-pedal** (`loadingPlugin.txt`): Written before load, cleared on success. Protects against SIGSEGV crashes that try/catch cannot catch. On next startup, `restoreFromStateXml` detects it and skips the crashy plugin.

## File Paths

All cached at `~/Library/Application Support/Chordy/`:
- `pluginList.xml` -- scan cache
- `deadMansPedal.txt` -- scan crash protection
- `loadingPlugin.txt` -- load crash protection

## Implementation Details

- Background scanning via `PluginDirectoryScanner` with results cached.
- Async loading with generation counter to prevent stale callbacks from racing.
- `setRateAndBufferSizeDetails()` called before `prepareToPlay()`.
- Plugins deduplicated by name in UI (VST3 preferred over AU).
- Plugin editor opens in a separate `DocumentWindow` (always-on-top). Editor window closed before loading a new plugin.
- Plugin state persisted as XML + base64 blob. Restore is async with try/catch around `setStateInformation` + `prepareToPlay`.

## Compile Definitions

`JUCE_PLUGINHOST_VST3=1`, `JUCE_PLUGINHOST_AU=1` in CMakeLists.txt.
