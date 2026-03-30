#include "VoicingModel.h"
#include <algorithm>
#include <numeric>

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
                                         const juce::String& name,
                                         const std::vector<int>& noteVelocities)
{
    Voicing v;
    v.id = juce::Uuid().toString();
    v.name = name;
    v.createdAt = juce::Time::currentTimeMillis();

    if (midiNotes.empty())
        return v;

    // Sort notes (and velocities in parallel) and compute intervals from the lowest note
    std::vector<size_t> indices (midiNotes.size());
    std::iota (indices.begin(), indices.end(), 0);
    std::sort (indices.begin(), indices.end(),
               [&midiNotes] (size_t a, size_t b) { return midiNotes[a] < midiNotes[b]; });

    int root = midiNotes[indices[0]];
    v.octaveReference = root;
    v.rootPitchClass = root % 12;

    for (size_t idx : indices)
    {
        v.intervals.push_back (midiNotes[idx] - root);
        v.velocities.push_back (idx < noteVelocities.size() ? noteVelocities[idx] : 100);
    }

    // Auto-detect quality using ChordDetector (root stays as lowest note;
    // users can override manually for rootless voicings)
    auto chordResult = ChordDetector::detect (midiNotes);
    v.quality = chordResult.quality;

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

// First occurrence of pitchClass strictly above 'above'
static int firstPcAbove (int pitchClass, int above)
{
    int target = above + 1 + ((pitchClass - (above + 1) % 12 + 120) % 12);
    return juce::jlimit (0, 127, target);
}

// First occurrence of pitchClass strictly below 'below'
static int firstPcBelow (int pitchClass, int below)
{
    int target = below - 1 - (((below - 1) % 12 - pitchClass + 12) % 12);
    return juce::jlimit (0, 127, target);
}

std::vector<int> VoicingLibrary::applyInversion (const std::vector<int>& notes, int inversion)
{
    if (notes.empty() || inversion <= 0 || inversion >= static_cast<int> (notes.size()))
        return notes;

    auto result = notes;
    std::sort (result.begin(), result.end());

    // Iteratively move the bottom note above the current highest
    for (int i = 0; i < inversion; ++i)
    {
        int bottomNote = result.front();
        int highest = result.back();
        int pc = bottomNote % 12;
        result.front() = firstPcAbove (pc, highest);
        std::sort (result.begin(), result.end());
    }
    return result;
}

std::vector<int> VoicingLibrary::applyDrop (const std::vector<int>& notes, const std::vector<int>& drops)
{
    int n = static_cast<int> (notes.size());
    if (n < 3 || drops.empty())
        return notes;

    auto result = notes;
    std::sort (result.begin(), result.end());

    // Process drops from highest position number first (lowest index in sorted array)
    // so that dropping one voice doesn't shift the indices of the others
    auto sortedDrops = drops;
    std::sort (sortedDrops.rbegin(), sortedDrops.rend());

    for (int dropN : sortedDrops)
    {
        if (dropN < 2 || dropN >= n) continue;
        size_t idx = static_cast<size_t> (n - dropN);
        int pc = result[idx] % 12;
        int lowest = result.front();
        result[idx] = firstPcBelow (pc, lowest);
        std::sort (result.begin(), result.end());
    }
    return result;
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
static const juce::Identifier ID_velocities ("velocities");
static const juce::Identifier ID_octaveRef ("octaveRef");
static const juce::Identifier ID_folderId ("folderId");
static const juce::Identifier ID_createdAt ("createdAt");

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
    tree.setProperty (ID_velocities, intervalsToString (v.velocities), nullptr);
    tree.setProperty (ID_octaveRef, v.octaveReference, nullptr);
    tree.setProperty (ID_folderId, v.folderId, nullptr);
    tree.setProperty (ID_createdAt, v.createdAt, nullptr);
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
    v.velocities = stringToIntervals (tree.getProperty (ID_velocities).toString());
    v.octaveReference = tree.getProperty (ID_octaveRef, 60);
    v.folderId = tree.getProperty (ID_folderId, "").toString();
    v.createdAt = static_cast<juce::int64> (tree.getProperty (ID_createdAt, 0));
    return v;
}

juce::ValueTree VoicingLibrary::toValueTree() const
{
    juce::ValueTree tree (ID_VoicingLibrary);
    for (const auto& v : voicings)
        tree.appendChild (voicingToValueTree (v), nullptr);
    tree.appendChild (folders.toValueTree(), nullptr);
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

    auto foldersTree = tree.getChildWithName ("Folders");
    if (foldersTree.isValid())
        folders.fromValueTree (foldersTree);
}
