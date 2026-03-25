#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "VoicingModel.h"
#include "SpacedRepetition.h"
#include "ChordDetector.h"

class AudioPluginAudioProcessor;
class ChordyKeyboardComponent;

class PracticePanel : public juce::Component
{
public:
    PracticePanel (AudioPluginAudioProcessor& processor,
                   ChordyKeyboardComponent& keyboard);

    void resized() override;
    void paint (juce::Graphics& g) override;

    // Call from timer to update practice state
    void updatePractice (const std::vector<int>& activeNotes);

    // Start practicing a specific voicing
    void startPractice (const juce::String& voicingId);

    // Stop practice mode
    void stopPractice();

    bool isPracticing() const { return practicing; }

private:
    AudioPluginAudioProcessor& processorRef;
    ChordyKeyboardComponent& keyboardRef;

    juce::Label headerLabel;
    juce::Label targetLabel;
    juce::Label feedbackLabel;
    juce::Label statsLabel;
    juce::TextButton startButton { "Start" };
    juce::TextButton nextButton { "Next" };
    juce::TextButton playButton { "Play" };

    bool practicing = false;
    PracticeChallenge currentChallenge;
    std::vector<int> targetNotes;   // absolute MIDI notes for current challenge
    bool challengeCompleted = false;
    double challengeCompletedTime = 0.0;

    void onStart();
    void onNext();
    void onPlay();
    void loadNextChallenge();
    void updateKeyboardColours (const std::vector<int>& activeNotes);
    void updateStats();
};
