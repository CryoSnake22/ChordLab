#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "ChordyTheme.h"

class ChordyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ChordyLookAndFeel();

    // Buttons
    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool isHighlighted, bool isDown) override;

    // Toggle buttons (pill switch)
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool isHighlighted, bool isDown) override;

    // Slider
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;

    // ComboBox
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;

    // TextEditor
    void fillTextEditorBackground (juce::Graphics&, int width, int height,
                                   juce::TextEditor&) override;
    void drawTextEditorOutline (juce::Graphics&, int width, int height,
                                juce::TextEditor&) override;

    // Tabs
    void drawTabButton (juce::TabBarButton&, juce::Graphics&,
                        bool isMouseOver, bool isMouseDown) override;
    void drawTabAreaBehindFrontButton (juce::TabbedButtonBar&, juce::Graphics&,
                                       int w, int h) override;
    void drawTabbedButtonBarBackground (juce::TabbedButtonBar&, juce::Graphics&) override;

    // Popup menu
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu,
                            const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;

    // Scrollbar
    void drawScrollbar (juce::Graphics&, juce::ScrollBar&,
                        int x, int y, int width, int height,
                        bool isVertical, int thumbStart, int thumbSize,
                        bool isMouseOver, bool isMouseDown) override;
    int getDefaultScrollbarWidth() override;
};
