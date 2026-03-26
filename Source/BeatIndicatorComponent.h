#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class BeatIndicatorComponent : public juce::Component
{
public:
    BeatIndicatorComponent() = default;

    void setBeatInfo (int beatNumber, float beatPhase, double bpm);
    void paint (juce::Graphics& g) override;

private:
    int currentBeat = 0;
    float currentPhase = 0.0f;
    double currentBpm = 120.0;
};
