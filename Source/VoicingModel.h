#pragma once

#include "ChordDetector.h"
#include <juce_data_structures/juce_data_structures.h>
#include <vector>

struct Voicing
{
    juce::String id;
    juce::String name;
    ChordQuality quality = ChordQuality::Unknown;
    std::vector<int> intervals;  // semitones from root, always starts with 0
    int octaveReference = 60;    // MIDI note of root when recorded

    bool isValid() const { return ! intervals.empty() && ! id.isEmpty(); }
};

class VoicingLibrary
{
public:
    VoicingLibrary() = default;

    // Library operations
    void addVoicing (const Voicing& v);
    void removeVoicing (const juce::String& id);
    const Voicing* getVoicing (const juce::String& id) const;
    const std::vector<Voicing>& getAllVoicings() const { return voicings; }
    std::vector<Voicing> getVoicingsByQuality (ChordQuality q) const;
    int size() const { return static_cast<int> (voicings.size()); }

    // Create a voicing from raw MIDI notes (auto-detects quality)
    static Voicing createFromNotes (const std::vector<int>& midiNotes,
                                    const juce::String& name);

    // Transpose a voicing to a specific root note, returning absolute MIDI notes
    static std::vector<int> transposeToKey (const Voicing& v, int rootMidiNote);

    // Serialization
    juce::ValueTree toValueTree() const;
    void fromValueTree (const juce::ValueTree& tree);

private:
    std::vector<Voicing> voicings;

    static juce::ValueTree voicingToValueTree (const Voicing& v);
    static Voicing voicingFromValueTree (const juce::ValueTree& tree);
};
