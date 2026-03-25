#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "VoicingModel.h"

// Forward declare
class AudioPluginAudioProcessor;

class VoicingLibraryPanel : public juce::Component,
                             private juce::ListBoxModel
{
public:
    VoicingLibraryPanel (AudioPluginAudioProcessor& processor);

    void resized() override;
    void paint (juce::Graphics& g) override;

    // Refresh the list from processor's library
    void refresh();

    // Called from editor's 60Hz timer with current active notes
    void updateRecording (const std::vector<int>& activeNotes);

    // Get the currently selected voicing ID (empty if none)
    juce::String getSelectedVoicingId() const;

    // Callback when selection changes
    std::function<void (const juce::String& voicingId)> onSelectionChanged;

private:
    // ListBoxModel
    int getNumRows() override;
    void paintListBoxItem (int rowNumber, juce::Graphics& g, int width, int height,
                           bool rowIsSelected) override;
    void selectedRowsChanged (int lastRowClicked) override;

    AudioPluginAudioProcessor& processorRef;

    juce::Label headerLabel;
    juce::Label recordingIndicator;
    juce::ComboBox qualityFilter;
    juce::ListBox voicingList;
    juce::TextButton recordButton { "Record" };
    juce::TextButton deleteButton { "Delete" };
    juce::TextEditor nameEditor;

    // Recording state machine
    enum class RecordState { Idle, Waiting, Capturing };
    RecordState recordState = RecordState::Idle;
    std::vector<int> capturedNotes;

    // Filtered list of voicings currently displayed
    std::vector<Voicing> displayedVoicings;

    void updateDisplayedVoicings();
    void onRecordToggle();
    void finishRecording();
    void cancelRecording();
    void onDelete();
};
