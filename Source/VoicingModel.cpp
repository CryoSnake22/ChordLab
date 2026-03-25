#include "VoicingModel.h"
#include <algorithm>

void VoicingLibrary::addVoicing (const Voicing& v)
{
    voicings.push_back (v);
}

void VoicingLibrary::removeVoicing (const juce::String& id)
{
    voicings.erase (
        std::remove_if (voicings.begin(), voicings.end(),
                        [&id] (const Voicing& v) { return v.id == id; }),
        voicings.end());
}

const Voicing* VoicingLibrary::getVoicing (const juce::String& id) const
{
    for (const auto& v : voicings)
        if (v.id == id)
            return &v;
    return nullptr;
}

std::vector<Voicing> VoicingLibrary::getVoicingsByQuality (ChordQuality q) const
{
    std::vector<Voicing> result;
    for (const auto& v : voicings)
        if (v.quality == q)
            result.push_back (v);
    return result;
}

Voicing VoicingLibrary::createFromNotes (const std::vector<int>& midiNotes,
                                         const juce::String& name)
{
    Voicing v;
    v.id = juce::Uuid().toString();
    v.name = name;

    if (midiNotes.empty())
        return v;

    // Sort notes and compute intervals from the lowest note
    auto sorted = midiNotes;
    std::sort (sorted.begin(), sorted.end());

    int root = sorted[0];
    v.octaveReference = root;
    v.rootPitchClass = root % 12;

    for (int note : sorted)
        v.intervals.push_back (note - root);

    // Auto-detect quality using ChordDetector
    auto chordResult = ChordDetector::detect (midiNotes);
    v.quality = chordResult.quality;
    if (chordResult.isValid())
        v.rootPitchClass = chordResult.rootPitchClass;

    return v;
}

std::vector<int> VoicingLibrary::transposeToKey (const Voicing& v,
                                                  int rootMidiNote)
{
    std::vector<int> notes;
    for (int interval : v.intervals)
        notes.push_back (rootMidiNote + interval);
    return notes;
}

const Voicing* VoicingLibrary::findByNotes (const std::vector<int>& midiNotes,
                                             juce::String& outDisplayName) const
{
    if (midiNotes.empty() || voicings.empty())
        return nullptr;

    // Compute intervals from lowest note
    auto sorted = midiNotes;
    std::sort (sorted.begin(), sorted.end());
    int bassNote = sorted[0];

    std::vector<int> playedIntervals;
    for (int note : sorted)
        playedIntervals.push_back (note - bassNote);

    // Check each voicing in the library
    for (const auto& v : voicings)
    {
        if (v.intervals == playedIntervals)
        {
            // Exact interval match — show the user's name with the current root
            juce::String rootName = ChordDetector::noteNameFromPitchClass (bassNote % 12);
            juce::String qualLabel = v.getQualityLabel();
            outDisplayName = rootName + qualLabel + " (" + v.name + ")";
            return &v;
        }
    }

    return nullptr;
}

// --- Serialization ---

static const juce::Identifier ID_VoicingLibrary ("VoicingLibrary");
static const juce::Identifier ID_Voicing ("Voicing");
static const juce::Identifier ID_id ("id");
static const juce::Identifier ID_name ("name");
static const juce::Identifier ID_quality ("quality");
static const juce::Identifier ID_alterations ("alterations");
static const juce::Identifier ID_rootPitchClass ("rootPitchClass");
static const juce::Identifier ID_intervals ("intervals");
static const juce::Identifier ID_octaveRef ("octaveRef");

static juce::String intervalsToString (const std::vector<int>& intervals)
{
    juce::StringArray parts;
    for (int i : intervals)
        parts.add (juce::String (i));
    return parts.joinIntoString (",");
}

static std::vector<int> stringToIntervals (const juce::String& s)
{
    std::vector<int> result;
    auto parts = juce::StringArray::fromTokens (s, ",", "");
    for (const auto& part : parts)
    {
        auto trimmed = part.trim();
        if (trimmed.isNotEmpty())
            result.push_back (trimmed.getIntValue());
    }
    return result;
}

juce::ValueTree VoicingLibrary::voicingToValueTree (const Voicing& v)
{
    juce::ValueTree tree (ID_Voicing);
    tree.setProperty (ID_id, v.id, nullptr);
    tree.setProperty (ID_name, v.name, nullptr);
    tree.setProperty (ID_quality, static_cast<int> (v.quality), nullptr);
    tree.setProperty (ID_alterations, v.alterations, nullptr);
    tree.setProperty (ID_rootPitchClass, v.rootPitchClass, nullptr);
    tree.setProperty (ID_intervals, intervalsToString (v.intervals), nullptr);
    tree.setProperty (ID_octaveRef, v.octaveReference, nullptr);
    return tree;
}

Voicing VoicingLibrary::voicingFromValueTree (const juce::ValueTree& tree)
{
    Voicing v;
    v.id = tree.getProperty (ID_id).toString();
    v.name = tree.getProperty (ID_name).toString();
    v.quality = static_cast<ChordQuality> (static_cast<int> (tree.getProperty (ID_quality)));
    v.alterations = tree.getProperty (ID_alterations).toString();
    v.rootPitchClass = tree.getProperty (ID_rootPitchClass, 0);
    v.intervals = stringToIntervals (tree.getProperty (ID_intervals).toString());
    v.octaveReference = tree.getProperty (ID_octaveRef, 60);
    return v;
}

juce::ValueTree VoicingLibrary::toValueTree() const
{
    juce::ValueTree tree (ID_VoicingLibrary);
    for (const auto& v : voicings)
        tree.appendChild (voicingToValueTree (v), nullptr);
    return tree;
}

void VoicingLibrary::fromValueTree (const juce::ValueTree& tree)
{
    voicings.clear();
    for (int i = 0; i < tree.getNumChildren(); ++i)
    {
        auto child = tree.getChild (i);
        if (child.hasType (ID_Voicing))
            voicings.push_back (voicingFromValueTree (child));
    }
}
