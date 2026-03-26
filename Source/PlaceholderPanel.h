#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ChordyTheme.h"

class PlaceholderPanel : public juce::Component
{
public:
    PlaceholderPanel (const juce::String& text) : labelText (text) {}

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (ChordyTheme::bgSurface));
        g.setColour (juce::Colour (ChordyTheme::textTertiary));
        g.setFont (juce::FontOptions (16.0f));
        g.drawText (labelText, getLocalBounds(), juce::Justification::centred);
    }

private:
    juce::String labelText;
};
