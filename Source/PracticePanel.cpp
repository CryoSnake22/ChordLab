#include "PracticePanel.h"
#include "PluginProcessor.h"
#include "ChordyKeyboardComponent.h"
#include <algorithm>
#include <set>

PracticePanel::PracticePanel (AudioPluginAudioProcessor& processor,
                               ChordyKeyboardComponent& keyboard)
    : processorRef (processor), keyboardRef (keyboard)
{
    headerLabel.setText ("PRACTICE", juce::dontSendNotification);
    headerLabel.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    headerLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (headerLabel);

    targetLabel.setText ("Select a voicing and press Start", juce::dontSendNotification);
    targetLabel.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    targetLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    targetLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (targetLabel);

    feedbackLabel.setFont (juce::FontOptions (14.0f));
    feedbackLabel.setColour (juce::Label::textColourId, juce::Colour (0xFFAABBCC));
    feedbackLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (feedbackLabel);

    statsLabel.setFont (juce::FontOptions (13.0f));
    statsLabel.setColour (juce::Label::textColourId, juce::Colour (0xFF889999));
    statsLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (statsLabel);

    startButton.onClick = [this] { onStart(); };
    addAndMakeVisible (startButton);

    nextButton.onClick = [this] { onNext(); };
    nextButton.setEnabled (false);
    addAndMakeVisible (nextButton);

    playButton.onClick = [this] { onPlay(); };
    playButton.setEnabled (false);
    addAndMakeVisible (playButton);

    updateStats();
}

void PracticePanel::paint (juce::Graphics& g)
{
    g.setColour (juce::Colour (0xFF222244));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
}

void PracticePanel::resized()
{
    auto area = getLocalBounds().reduced (8);

    headerLabel.setBounds (area.removeFromTop (24));
    area.removeFromTop (8);

    targetLabel.setBounds (area.removeFromTop (36));
    area.removeFromTop (4);

    auto buttonRow = area.removeFromTop (30);
    int buttonWidth = (buttonRow.getWidth() - 8) / 3;
    startButton.setBounds (buttonRow.removeFromLeft (buttonWidth));
    buttonRow.removeFromLeft (4);
    nextButton.setBounds (buttonRow.removeFromLeft (buttonWidth));
    buttonRow.removeFromLeft (4);
    playButton.setBounds (buttonRow);

    area.removeFromTop (8);
    feedbackLabel.setBounds (area.removeFromTop (24));
    area.removeFromTop (4);
    statsLabel.setBounds (area.removeFromTop (24));
}

void PracticePanel::startPractice (const juce::String& voicingId)
{
    practicing = true;
    challengeCompleted = false;

    currentChallenge = processorRef.spacedRepetition.getNextChallenge (voicingId);

    const auto* voicing = processorRef.voicingLibrary.getVoicing (voicingId);
    if (voicing == nullptr)
    {
        stopPractice();
        return;
    }

    targetNotes = VoicingLibrary::transposeToKey (*voicing, currentChallenge.rootMidiNote);

    juce::String keyName = ChordDetector::noteNameFromPitchClass (currentChallenge.keyIndex);
    targetLabel.setText ("Play: " + keyName + ChordDetector::qualitySuffix (voicing->quality),
                         juce::dontSendNotification);
    feedbackLabel.setText ("", juce::dontSendNotification);

    nextButton.setEnabled (true);
    playButton.setEnabled (true);

    // Show target notes on keyboard
    keyboardRef.clearAllColours();
    for (int note : targetNotes)
        keyboardRef.setKeyColour (note, KeyColour::Target);
    keyboardRef.repaint();
}

void PracticePanel::stopPractice()
{
    practicing = false;
    challengeCompleted = false;
    targetNotes.clear();
    targetLabel.setText ("Select a voicing and press Start", juce::dontSendNotification);
    feedbackLabel.setText ("", juce::dontSendNotification);
    nextButton.setEnabled (false);
    playButton.setEnabled (false);
    keyboardRef.clearAllColours();
    keyboardRef.repaint();
}

void PracticePanel::updatePractice (const std::vector<int>& activeNotes)
{
    if (! practicing || targetNotes.empty())
        return;

    // Auto-advance after success
    if (challengeCompleted)
    {
        double now = juce::Time::currentTimeMillis() / 1000.0;
        if (now - challengeCompletedTime > 1.0)
            loadNextChallenge();
        return;
    }

    if (activeNotes.empty())
    {
        // Show target notes when nothing is pressed
        keyboardRef.clearAllColours();
        for (int note : targetNotes)
            keyboardRef.setKeyColour (note, KeyColour::Target);
        keyboardRef.repaint();
        return;
    }

    updateKeyboardColours (activeNotes);

    // Check if user played correctly — compare pitch classes
    std::set<int> targetPitchClasses;
    for (int n : targetNotes)
        targetPitchClasses.insert (n % 12);

    std::set<int> playedPitchClasses;
    for (int n : activeNotes)
        playedPitchClasses.insert (n % 12);

    if (playedPitchClasses == targetPitchClasses)
    {
        challengeCompleted = true;
        challengeCompletedTime = juce::Time::currentTimeMillis() / 1000.0;
        processorRef.spacedRepetition.recordSuccess (
            currentChallenge.voicingId, currentChallenge.keyIndex);
        feedbackLabel.setText ("Correct!", juce::dontSendNotification);
        feedbackLabel.setColour (juce::Label::textColourId, juce::Colour (0xFF00CC44));
        updateStats();
    }
}

void PracticePanel::loadNextChallenge()
{
    challengeCompleted = false;
    startPractice (currentChallenge.voicingId);
}

void PracticePanel::updateKeyboardColours (const std::vector<int>& activeNotes)
{
    keyboardRef.clearAllColours();

    std::set<int> targetPitchClasses;
    for (int n : targetNotes)
        targetPitchClasses.insert (n % 12);

    std::set<int> playedPitchClasses;
    for (int n : activeNotes)
        playedPitchClasses.insert (n % 12);

    // Mark played notes
    for (int note : activeNotes)
    {
        int pc = note % 12;
        if (targetPitchClasses.count (pc) > 0)
            keyboardRef.setKeyColour (note, KeyColour::Correct);
        else
            keyboardRef.setKeyColour (note, KeyColour::Wrong);
    }

    // Mark unplayed target notes
    for (int note : targetNotes)
    {
        int pc = note % 12;
        if (playedPitchClasses.count (pc) == 0)
            keyboardRef.setKeyColour (note, KeyColour::Target);
    }

    keyboardRef.repaint();
}

void PracticePanel::updateStats()
{
    int attempts = processorRef.spacedRepetition.getTotalAttempts();
    int successes = processorRef.spacedRepetition.getTotalSuccesses();
    double accuracy = processorRef.spacedRepetition.getAccuracy();

    juce::String statsText = "Accuracy: " + juce::String (juce::roundToInt (accuracy * 100))
                             + "% (" + juce::String (successes) + "/" + juce::String (attempts) + ")";
    statsLabel.setText (statsText, juce::dontSendNotification);
}

void PracticePanel::onStart()
{
    // Use currently selected voicing, or first available
    auto& lib = processorRef.voicingLibrary;
    if (lib.size() == 0)
    {
        targetLabel.setText ("Record a voicing first!", juce::dontSendNotification);
        return;
    }

    // Default to first voicing if none selected externally
    startPractice (lib.getAllVoicings()[0].id);
}

void PracticePanel::onNext()
{
    if (! practicing)
        return;

    // Mark as failure if not completed
    if (! challengeCompleted)
    {
        processorRef.spacedRepetition.recordFailure (
            currentChallenge.voicingId, currentChallenge.keyIndex);
        feedbackLabel.setColour (juce::Label::textColourId, juce::Colour (0xFFCC2200));
        updateStats();
    }

    loadNextChallenge();
}

void PracticePanel::onPlay()
{
    if (targetNotes.empty())
        return;

    // Play the target voicing through the keyboard state
    int channel = static_cast<int> (*processorRef.apvts.getRawParameterValue ("midiChannel"));
    for (int note : targetNotes)
        processorRef.keyboardState.noteOn (channel, note, 0.8f);

    // Schedule note offs after 800ms
    juce::Timer::callAfterDelay (800, [this, channel]
    {
        for (int note : targetNotes)
            processorRef.keyboardState.noteOff (channel, note, 0.0f);
    });
}
