#include "PluginEditor.h"
#include "ChordDetector.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(
    AudioPluginAudioProcessor &p)
    : AudioProcessorEditor(&p), processorRef(p),
      keyboard(p.keyboardState,
               juce::MidiKeyboardComponent::horizontalKeyboard),
      voicingLibraryPanel(p),
      practicePanel(p, keyboard) {

  // Title
  titleLabel.setText("CHORDY", juce::dontSendNotification);
  titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
  titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  addAndMakeVisible(titleLabel);

  // Chord display
  chordDisplayLabel.setText("Play something...", juce::dontSendNotification);
  chordDisplayLabel.setFont(juce::FontOptions(36.0f, juce::Font::bold));
  chordDisplayLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  chordDisplayLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(chordDisplayLabel);

  // Keyboard
  keyboard.setAvailableRange(36, 96); // C2 to C7
  addAndMakeVisible(keyboard);

  // Voicing library panel
  voicingLibraryPanel.onSelectionChanged = [this](const juce::String &voicingId) {
    if (voicingId.isNotEmpty() && practicePanel.isPracticing())
      practicePanel.startPractice(voicingId);
  };
  addAndMakeVisible(voicingLibraryPanel);

  // Practice panel
  addAndMakeVisible(practicePanel);

  setSize(800, 600);
  startTimerHz(60);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {
  stopTimer();
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xFF1A1A2E));
}

void AudioPluginAudioProcessorEditor::resized() {
  auto area = getLocalBounds();

  // Header
  auto headerArea = area.removeFromTop(40);
  titleLabel.setBounds(headerArea.reduced(10, 5));

  // Chord display
  auto chordArea = area.removeFromTop(60);
  chordDisplayLabel.setBounds(chordArea.reduced(10, 5));

  // Keyboard
  auto keyboardArea = area.removeFromTop(100);
  keyboard.setBounds(keyboardArea.reduced(8));

  // Bottom panels: library on left, practice on right
  auto bottomArea = area.reduced(8);
  auto libraryArea = bottomArea.removeFromLeft(getWidth() / 2 - 12);
  bottomArea.removeFromLeft(8);
  voicingLibraryPanel.setBounds(libraryArea);
  practicePanel.setBounds(bottomArea);
}

void AudioPluginAudioProcessorEditor::timerCallback() {
  auto notes = processorRef.getActiveNotes();

  // Update chord display — use last-played notes when keys are released
  auto displayNotes = notes.empty() ? processorRef.getLastPlayedNotes() : notes;

  if (displayNotes.empty()) {
    chordDisplayLabel.setText("Play something...", juce::dontSendNotification);
  } else {
    auto result = ChordDetector::detect(displayNotes);
    if (result.isValid())
      chordDisplayLabel.setText(result.displayName, juce::dontSendNotification);
    else
      chordDisplayLabel.setText("...", juce::dontSendNotification);
  }

  // Update practice mode
  if (practicePanel.isPracticing())
    practicePanel.updatePractice(notes);
}
