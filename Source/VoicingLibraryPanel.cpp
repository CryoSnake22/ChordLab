#include "VoicingLibraryPanel.h"
#include "PluginProcessor.h"
#include "ChordDetector.h"

VoicingLibraryPanel::VoicingLibraryPanel (AudioPluginAudioProcessor& processor)
    : processorRef (processor)
{
    headerLabel.setText ("VOICING LIBRARY", juce::dontSendNotification);
    headerLabel.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    headerLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (headerLabel);

    // Recording indicator (hidden by default)
    recordingIndicator.setText ("REC", juce::dontSendNotification);
    recordingIndicator.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    recordingIndicator.setColour (juce::Label::textColourId, juce::Colour (0xFFFF3333));
    recordingIndicator.setJustificationType (juce::Justification::centredRight);
    recordingIndicator.setVisible (false);
    addAndMakeVisible (recordingIndicator);

    // Quality filter
    qualityFilter.addItem ("All", 1);
    qualityFilter.addItem ("Major", 2);
    qualityFilter.addItem ("Minor", 3);
    qualityFilter.addItem ("Dom7", 4);
    qualityFilter.addItem ("Maj7", 5);
    qualityFilter.addItem ("Min7", 6);
    qualityFilter.addItem ("Dim", 7);
    qualityFilter.addItem ("Other", 8);
    qualityFilter.setSelectedId (1);
    qualityFilter.onChange = [this] { updateDisplayedVoicings(); };
    addAndMakeVisible (qualityFilter);

    // List
    voicingList.setModel (this);
    voicingList.setColour (juce::ListBox::backgroundColourId, juce::Colour (0xFF2A2A3E));
    voicingList.setColour (juce::ListBox::outlineColourId, juce::Colour (0xFF444466));
    voicingList.setOutlineThickness (1);
    addAndMakeVisible (voicingList);

    // Name editor for new voicings
    nameEditor.setTextToShowWhenEmpty ("Voicing name...", juce::Colours::grey);
    addAndMakeVisible (nameEditor);

    // Buttons
    recordButton.onClick = [this] { onRecordToggle(); };
    addAndMakeVisible (recordButton);

    deleteButton.onClick = [this] { onDelete(); };
    addAndMakeVisible (deleteButton);

    updateDisplayedVoicings();
}

void VoicingLibraryPanel::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour (0xFF222244));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);

    // Red border when recording
    if (recordState != RecordState::Idle)
    {
        g.setColour (juce::Colour (0xFFFF3333));
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 6.0f, 2.0f);
    }
}

void VoicingLibraryPanel::resized()
{
    auto area = getLocalBounds().reduced (8);

    auto headerRow = area.removeFromTop (24);
    headerLabel.setBounds (headerRow.removeFromLeft (headerRow.getWidth() / 2));
    recordingIndicator.setBounds (headerRow);
    area.removeFromTop (4);

    auto filterRow = area.removeFromTop (28);
    qualityFilter.setBounds (filterRow);
    area.removeFromTop (4);

    auto bottomRow = area.removeFromBottom (30);
    deleteButton.setBounds (bottomRow.removeFromRight (60));
    bottomRow.removeFromRight (4);
    recordButton.setBounds (bottomRow.removeFromRight (70));
    area.removeFromBottom (4);

    auto nameRow = area.removeFromBottom (28);
    nameEditor.setBounds (nameRow);
    area.removeFromBottom (4);

    voicingList.setBounds (area);
}

void VoicingLibraryPanel::refresh()
{
    updateDisplayedVoicings();
}

void VoicingLibraryPanel::updateRecording (const std::vector<int>& activeNotes)
{
    if (recordState == RecordState::Idle)
        return;

    if (recordState == RecordState::Waiting)
    {
        // Waiting for the user to start playing
        if (! activeNotes.empty())
        {
            recordState = RecordState::Capturing;
            capturedNotes = activeNotes;
            recordingIndicator.setText ("REC - Playing...", juce::dontSendNotification);
        }
        return;
    }

    if (recordState == RecordState::Capturing)
    {
        if (! activeNotes.empty())
        {
            // Update captured notes to the current set (user might adjust fingers)
            capturedNotes = activeNotes;
        }
        else
        {
            // All notes released — finish recording
            finishRecording();
        }
    }
}

juce::String VoicingLibraryPanel::getSelectedVoicingId() const
{
    int row = voicingList.getSelectedRow();
    if (row >= 0 && row < static_cast<int> (displayedVoicings.size()))
        return displayedVoicings[static_cast<size_t> (row)].id;
    return {};
}

void VoicingLibraryPanel::updateDisplayedVoicings()
{
    int filterId = qualityFilter.getSelectedId();
    const auto& all = processorRef.voicingLibrary.getAllVoicings();

    displayedVoicings.clear();

    if (filterId == 1) // All
    {
        displayedVoicings = all;
    }
    else
    {
        ChordQuality filterQuality = ChordQuality::Unknown;
        switch (filterId)
        {
            case 2: filterQuality = ChordQuality::Major; break;
            case 3: filterQuality = ChordQuality::Minor; break;
            case 4: filterQuality = ChordQuality::Dom7; break;
            case 5: filterQuality = ChordQuality::Maj7; break;
            case 6: filterQuality = ChordQuality::Min7; break;
            case 7: filterQuality = ChordQuality::Diminished; break;
            default: break;
        }

        for (const auto& v : all)
        {
            if (filterId == 8) // "Other"
            {
                if (v.quality != ChordQuality::Major && v.quality != ChordQuality::Minor &&
                    v.quality != ChordQuality::Dom7 && v.quality != ChordQuality::Maj7 &&
                    v.quality != ChordQuality::Min7 && v.quality != ChordQuality::Diminished)
                    displayedVoicings.push_back (v);
            }
            else if (v.quality == filterQuality)
            {
                displayedVoicings.push_back (v);
            }
        }
    }

    voicingList.updateContent();
    voicingList.repaint();
}

void VoicingLibraryPanel::onRecordToggle()
{
    if (recordState != RecordState::Idle)
    {
        // Cancel recording
        cancelRecording();
        return;
    }

    // Enter recording mode
    recordState = RecordState::Waiting;
    capturedNotes.clear();
    recordButton.setButtonText ("Cancel");
    recordButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF882222));
    recordingIndicator.setText ("REC - Play a chord...", juce::dontSendNotification);
    recordingIndicator.setVisible (true);
    repaint();
}

void VoicingLibraryPanel::finishRecording()
{
    if (capturedNotes.empty())
    {
        cancelRecording();
        return;
    }

    juce::String name = nameEditor.getText().trim();
    if (name.isEmpty())
    {
        auto result = ChordDetector::detect (capturedNotes);
        if (result.isValid())
            name = result.displayName + " voicing";
        else
            name = "Voicing " + juce::String (processorRef.voicingLibrary.size() + 1);
    }

    auto voicing = VoicingLibrary::createFromNotes (capturedNotes, name);
    processorRef.voicingLibrary.addVoicing (voicing);

    nameEditor.clear();
    capturedNotes.clear();

    // Reset UI
    recordState = RecordState::Idle;
    recordButton.setButtonText ("Record");
    recordButton.setColour (juce::TextButton::buttonColourId,
                            getLookAndFeel().findColour (juce::TextButton::buttonColourId));
    recordingIndicator.setVisible (false);
    repaint();

    updateDisplayedVoicings();
}

void VoicingLibraryPanel::cancelRecording()
{
    recordState = RecordState::Idle;
    capturedNotes.clear();
    recordButton.setButtonText ("Record");
    recordButton.setColour (juce::TextButton::buttonColourId,
                            getLookAndFeel().findColour (juce::TextButton::buttonColourId));
    recordingIndicator.setVisible (false);
    repaint();
}

void VoicingLibraryPanel::onDelete()
{
    auto id = getSelectedVoicingId();
    if (id.isNotEmpty())
    {
        processorRef.voicingLibrary.removeVoicing (id);
        updateDisplayedVoicings();
    }
}

// --- ListBoxModel ---

int VoicingLibraryPanel::getNumRows()
{
    return static_cast<int> (displayedVoicings.size());
}

void VoicingLibraryPanel::paintListBoxItem (int rowNumber, juce::Graphics& g,
                                             int width, int height,
                                             bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= static_cast<int> (displayedVoicings.size()))
        return;

    const auto& v = displayedVoicings[static_cast<size_t> (rowNumber)];

    if (rowIsSelected)
        g.fillAll (juce::Colour (0xFF3A3A5E));
    else
        g.fillAll (juce::Colour (0xFF2A2A3E));

    g.setColour (juce::Colours::white);
    g.setFont (14.0f);
    g.drawText (v.name, 8, 0, width - 80, height, juce::Justification::centredLeft);

    // Show quality badge
    g.setColour (juce::Colour (0xFF88AACC));
    g.setFont (11.0f);
    g.drawText (ChordDetector::qualitySuffix (v.quality),
                width - 70, 0, 62, height, juce::Justification::centredRight);
}

void VoicingLibraryPanel::selectedRowsChanged (int /*lastRowClicked*/)
{
    if (onSelectionChanged)
        onSelectionChanged (getSelectedVoicingId());
}
