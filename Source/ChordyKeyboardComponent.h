#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <map>

enum class KeyColour
{
    Normal,
    Correct,   // green
    Wrong,     // red
    Target     // dim blue outline — note expected but not yet played
};

class ChordyKeyboardComponent : public juce::MidiKeyboardComponent
{
public:
    ChordyKeyboardComponent (juce::MidiKeyboardState& state,
                             Orientation orientation);

    void setKeyColour (int noteNumber, KeyColour colour);
    void clearAllColours();

protected:
    void drawWhiteNote (int midiNoteNumber, juce::Graphics& g, juce::Rectangle<float> area,
                        bool isDown, bool isOver, juce::Colour lineColour,
                        juce::Colour textColour) override;

    void drawBlackNote (int midiNoteNumber, juce::Graphics& g, juce::Rectangle<float> area,
                        bool isDown, bool isOver, juce::Colour noteFillColour) override;

private:
    std::map<int, KeyColour> keyColours;

    juce::Colour getOverlayColour (KeyColour kc) const;
};
