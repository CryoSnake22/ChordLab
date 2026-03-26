#include "BeatIndicatorComponent.h"
#include "ChordyTheme.h"

void BeatIndicatorComponent::setBeatInfo (int beatNumber, float beatPhase, double bpm)
{
    currentBeat = beatNumber;
    currentPhase = beatPhase;
    currentBpm = bpm;
}

void BeatIndicatorComponent::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    float dotSize = 12.0f;
    float spacing = 8.0f;
    float totalDotsWidth = 4 * dotSize + 3 * spacing;

    float startX = area.getX() + 4.0f;
    float centreY = area.getCentreY();

    for (int i = 0; i < 4; ++i)
    {
        float x = startX + static_cast<float> (i) * (dotSize + spacing);
        auto dotBounds = juce::Rectangle<float> (x, centreY - dotSize / 2, dotSize, dotSize);

        if (i == currentBeat)
        {
            // Active beat — pulse size slightly based on phase
            float pulse = 1.0f + 0.2f * (1.0f - currentPhase);
            float pulsedSize = dotSize * pulse;
            dotBounds = juce::Rectangle<float> (
                x - (pulsedSize - dotSize) / 2,
                centreY - pulsedSize / 2,
                pulsedSize, pulsedSize);

            // Downbeat vs upbeat colour
            auto colour = (i == 0) ? juce::Colour (ChordyTheme::beatDownbeat) : juce::Colour (ChordyTheme::beatUpbeat);
            g.setColour (colour);
        }
        else
        {
            g.setColour (juce::Colour (ChordyTheme::beatInactive));
        }

        g.fillEllipse (dotBounds);
    }

    // BPM text
    float textX = startX + totalDotsWidth + 12.0f;
    g.setColour (juce::Colour (ChordyTheme::textSecondary));
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (juce::String (juce::roundToInt (currentBpm)) + " BPM",
                static_cast<int> (textX),
                static_cast<int> (centreY - 10),
                100, 20,
                juce::Justification::centredLeft);
}
