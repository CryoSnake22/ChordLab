#include "ChordyLookAndFeel.h"

ChordyLookAndFeel::ChordyLookAndFeel()
{
    // Set up the ColourScheme
    auto scheme = juce::LookAndFeel_V4::ColourScheme {
        juce::Colour (ChordyTheme::bgDeepest),     // windowBackground
        juce::Colour (ChordyTheme::bgElevated),     // widgetBackground
        juce::Colour (ChordyTheme::bgSurface),      // menuBackground
        juce::Colour (ChordyTheme::border),          // outline
        juce::Colour (ChordyTheme::textPrimary),     // defaultText
        juce::Colour (ChordyTheme::accent),          // defaultFill
        juce::Colour (0xFFFFFFFF),                   // highlightedText
        juce::Colour (ChordyTheme::accent),          // highlightedFill
        juce::Colour (ChordyTheme::textPrimary)      // menuText
    };
    setColourScheme (scheme);

    // Global font
    setDefaultSansSerifTypefaceName (ChordyTheme::fontFamily);

    // Component-specific colour overrides
    setColour (juce::TextButton::buttonColourId,       juce::Colour (ChordyTheme::bgElevated));
    setColour (juce::TextButton::textColourOffId,      juce::Colour (ChordyTheme::textPrimary));
    setColour (juce::TextButton::textColourOnId,       juce::Colour (ChordyTheme::textPrimary));

    setColour (juce::ToggleButton::textColourId,       juce::Colour (ChordyTheme::textSecondary));
    setColour (juce::ToggleButton::tickColourId,       juce::Colour (ChordyTheme::accent));
    setColour (juce::ToggleButton::tickDisabledColourId, juce::Colour (ChordyTheme::borderFocus));

    setColour (juce::Slider::backgroundColourId,       juce::Colour (ChordyTheme::border));
    setColour (juce::Slider::trackColourId,            juce::Colour (ChordyTheme::accent));
    setColour (juce::Slider::thumbColourId,            juce::Colour (ChordyTheme::textPrimary));
    setColour (juce::Slider::textBoxTextColourId,      juce::Colour (ChordyTheme::textSecondary));
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0x00000000));
    setColour (juce::Slider::textBoxOutlineColourId,   juce::Colour (0x00000000));

    setColour (juce::ComboBox::backgroundColourId,     juce::Colour (ChordyTheme::bgElevated));
    setColour (juce::ComboBox::outlineColourId,        juce::Colour (ChordyTheme::border));
    setColour (juce::ComboBox::textColourId,           juce::Colour (ChordyTheme::textPrimary));
    setColour (juce::ComboBox::arrowColourId,          juce::Colour (ChordyTheme::textSecondary));

    setColour (juce::ListBox::backgroundColourId,      juce::Colour (ChordyTheme::bgSurface));
    setColour (juce::ListBox::outlineColourId,         juce::Colour (ChordyTheme::border));

    setColour (juce::TextEditor::backgroundColourId,   juce::Colour (ChordyTheme::bgElevated));
    setColour (juce::TextEditor::outlineColourId,      juce::Colour (ChordyTheme::border));
    setColour (juce::TextEditor::focusedOutlineColourId, juce::Colour (ChordyTheme::accent));
    setColour (juce::TextEditor::textColourId,         juce::Colour (ChordyTheme::textPrimary));

    setColour (juce::PopupMenu::backgroundColourId,    juce::Colour (ChordyTheme::bgSurface));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (ChordyTheme::bgSelected));
    setColour (juce::PopupMenu::textColourId,          juce::Colour (ChordyTheme::textPrimary));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xFFFFFFFF));

    setColour (juce::Label::textColourId,              juce::Colour (ChordyTheme::textPrimary));

    setColour (juce::TabbedButtonBar::tabOutlineColourId, juce::Colour (0x00000000));
    setColour (juce::TabbedComponent::outlineColourId, juce::Colour (0x00000000));

    setColour (juce::ScrollBar::thumbColourId,         juce::Colour (ChordyTheme::textTertiary));

    // Keyboard colours
    setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId,
               juce::Colour (ChordyTheme::keyDown));
    setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
               juce::Colour (ChordyTheme::keyHover));
    setColour (juce::MidiKeyboardComponent::whiteNoteColourId,
               juce::Colour (0xFFE0E0E0));
    setColour (juce::MidiKeyboardComponent::blackNoteColourId,
               juce::Colour (0xFF1A1A1A));
    setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId,
               juce::Colour (ChordyTheme::border));
}

//==============================================================================
// Buttons — flat rounded rectangles
void ChordyLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                               const juce::Colour& backgroundColour,
                                               bool isHighlighted, bool isDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    auto baseColour = backgroundColour;

    if (! button.isEnabled())
        baseColour = baseColour.withAlpha (0.4f);
    else if (isDown)
        baseColour = baseColour.darker (0.1f);
    else if (isHighlighted)
        baseColour = baseColour.brighter (0.08f);

    g.setColour (baseColour);
    g.fillRoundedRectangle (bounds, ChordyTheme::buttonRadius);
}

//==============================================================================
// Toggle buttons — pill-shaped switch
void ChordyLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                           bool isHighlighted, bool isDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto fontSize = juce::jmin (13.0f, bounds.getHeight() * 0.6f);

    // Pill dimensions
    float pillW = 28.0f;
    float pillH = 14.0f;
    float pillX = 4.0f;
    float pillY = (bounds.getHeight() - pillH) * 0.5f;
    float pillRadius = pillH * 0.5f;

    bool isOn = button.getToggleState();

    // Pill background
    auto pillColour = isOn ? juce::Colour (ChordyTheme::accent)
                           : juce::Colour (ChordyTheme::bgElevated);
    if (isHighlighted && button.isEnabled())
        pillColour = pillColour.brighter (0.08f);

    g.setColour (pillColour);
    g.fillRoundedRectangle (pillX, pillY, pillW, pillH, pillRadius);

    if (! isOn)
    {
        g.setColour (juce::Colour (ChordyTheme::border));
        g.drawRoundedRectangle (pillX, pillY, pillW, pillH, pillRadius, 1.0f);
    }

    // Circle thumb
    float circleD = pillH - 4.0f;
    float circleY = pillY + 2.0f;
    float circleX = isOn ? (pillX + pillW - circleD - 2.0f) : (pillX + 2.0f);

    g.setColour (juce::Colour (ChordyTheme::textPrimary));
    g.fillEllipse (circleX, circleY, circleD, circleD);

    // Label text
    auto textArea = bounds.withLeft (pillX + pillW + 6.0f);
    g.setColour (button.findColour (juce::ToggleButton::textColourId));
    g.setFont (juce::FontOptions (fontSize));
    g.drawText (button.getButtonText(), textArea.toNearestInt(),
                juce::Justification::centredLeft, true);
}

//==============================================================================
// Slider — thin track, small thumb
void ChordyLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y,
                                           int width, int height, float sliderPos,
                                           float minSliderPos, float maxSliderPos,
                                           juce::Slider::SliderStyle style,
                                           juce::Slider& slider)
{
    if (style != juce::Slider::LinearHorizontal)
    {
        juce::LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos,
                                                 minSliderPos, maxSliderPos, style, slider);
        return;
    }

    float trackH = 3.0f;
    float centreY = (float) y + (float) height * 0.5f;
    float trackY = centreY - trackH * 0.5f;
    float trackL = (float) x;
    float trackR = (float) x + (float) width;

    // Background track
    g.setColour (slider.findColour (juce::Slider::backgroundColourId));
    g.fillRoundedRectangle (trackL, trackY, trackR - trackL, trackH, trackH * 0.5f);

    // Value track
    g.setColour (slider.findColour (juce::Slider::trackColourId));
    g.fillRoundedRectangle (trackL, trackY, sliderPos - trackL, trackH, trackH * 0.5f);

    // Thumb
    float thumbR = 5.0f;
    g.setColour (slider.findColour (juce::Slider::thumbColourId));
    g.fillEllipse (sliderPos - thumbR, centreY - thumbR, thumbR * 2.0f, thumbR * 2.0f);
}

//==============================================================================
// ComboBox — clean rounded dropdown
void ChordyLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height,
                                       bool isButtonDown, int, int, int, int,
                                       juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<int> (0, 0, width, height).toFloat();

    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (bounds, ChordyTheme::smallRadius);

    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (bounds.reduced (0.5f), ChordyTheme::smallRadius, 1.0f);

    // Chevron arrow
    float arrowH = 5.0f;
    float arrowW = 8.0f;
    float arrowX = (float) width - 16.0f;
    float arrowY = ((float) height - arrowH) * 0.5f;

    juce::Path arrow;
    arrow.addTriangle (arrowX, arrowY,
                       arrowX + arrowW, arrowY,
                       arrowX + arrowW * 0.5f, arrowY + arrowH);

    g.setColour (box.findColour (juce::ComboBox::arrowColourId));
    g.fillPath (arrow);
}

//==============================================================================
// TextEditor
void ChordyLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int width,
                                                    int height, juce::TextEditor& editor)
{
    g.setColour (editor.findColour (juce::TextEditor::backgroundColourId));
    g.fillRoundedRectangle (0.0f, 0.0f, (float) width, (float) height,
                            ChordyTheme::smallRadius);
}

void ChordyLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int width,
                                                int height, juce::TextEditor& editor)
{
    auto colour = editor.hasKeyboardFocus (true)
                    ? editor.findColour (juce::TextEditor::focusedOutlineColourId)
                    : editor.findColour (juce::TextEditor::outlineColourId);
    float thickness = editor.hasKeyboardFocus (true) ? 1.5f : 1.0f;

    g.setColour (colour);
    g.drawRoundedRectangle (0.5f, 0.5f, (float) width - 1.0f, (float) height - 1.0f,
                            ChordyTheme::smallRadius, thickness);
}

//==============================================================================
// Tabs — flat underline style
void ChordyLookAndFeel::drawTabButton (juce::TabBarButton& button, juce::Graphics& g,
                                        bool isMouseOver, bool isMouseDown)
{
    auto area = button.getActiveArea();
    bool isFront = button.isFrontTab();

    // Text
    auto textColour = isFront ? juce::Colour (ChordyTheme::textPrimary)
                              : (isMouseOver ? juce::Colour (ChordyTheme::textSecondary)
                                             : juce::Colour (ChordyTheme::textTertiary));

    auto font = juce::FontOptions (ChordyTheme::fontSectionHead);
    g.setColour (textColour);
    g.setFont (font);
    g.drawText (button.getButtonText(), area, juce::Justification::centred, true);

    // Underline for active tab
    if (isFront)
    {
        g.setColour (juce::Colour (ChordyTheme::accent));
        g.fillRect (area.getX(), area.getBottom() - 2, area.getWidth(), 2);
    }
}

void ChordyLookAndFeel::drawTabAreaBehindFrontButton (juce::TabbedButtonBar& bar,
                                                       juce::Graphics& g, int w, int h)
{
    g.setColour (juce::Colour (ChordyTheme::border));
    g.drawHorizontalLine (h - 1, 0.0f, (float) w);
}

void ChordyLookAndFeel::drawTabbedButtonBarBackground (juce::TabbedButtonBar&,
                                                        juce::Graphics&)
{
    // Transparent — no fill
}

//==============================================================================
// Popup menu
void ChordyLookAndFeel::drawPopupMenuItem (juce::Graphics& g,
                                            const juce::Rectangle<int>& area,
                                            bool isSeparator, bool isActive,
                                            bool isHighlighted, bool isTicked,
                                            bool hasSubMenu,
                                            const juce::String& text,
                                            const juce::String& shortcutKeyText,
                                            const juce::Drawable* icon,
                                            const juce::Colour* textColour)
{
    if (isSeparator)
    {
        auto r = area.reduced (4, 0);
        g.setColour (juce::Colour (ChordyTheme::border));
        g.fillRect (r.removeFromTop (r.getHeight() / 2).removeFromBottom (1));
        return;
    }

    auto bgColour = isHighlighted ? juce::Colour (ChordyTheme::bgSelected)
                                  : juce::Colour (ChordyTheme::bgSurface);
    g.setColour (bgColour);
    g.fillRect (area);

    auto txtColour = textColour != nullptr ? *textColour
                     : (isActive ? juce::Colour (ChordyTheme::textPrimary)
                                 : juce::Colour (ChordyTheme::textTertiary));
    g.setColour (txtColour);
    g.setFont (juce::FontOptions (ChordyTheme::fontBody));

    auto textArea = area.reduced (12, 0);
    g.drawText (text, textArea, juce::Justification::centredLeft, true);

    if (isTicked)
    {
        g.setColour (juce::Colour (ChordyTheme::accent));
        auto tickArea = area.withLeft (area.getRight() - area.getHeight()).reduced (6);
        g.fillEllipse (tickArea.toFloat());
    }
}

//==============================================================================
// Scrollbar — thin rounded pill
void ChordyLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&,
                                        int x, int y, int width, int height,
                                        bool isVertical, int thumbStart, int thumbSize,
                                        bool isMouseOver, bool isMouseDown)
{
    auto thumbColour = juce::Colour (ChordyTheme::textTertiary)
                         .withAlpha (isMouseOver || isMouseDown ? 0.7f : 0.4f);
    g.setColour (thumbColour);

    if (isVertical)
    {
        float thumbW = juce::jmin ((float) width, 6.0f);
        float thumbX = (float) x + ((float) width - thumbW) * 0.5f;
        g.fillRoundedRectangle (thumbX, (float) thumbStart, thumbW,
                                (float) thumbSize, thumbW * 0.5f);
    }
    else
    {
        float thumbH = juce::jmin ((float) height, 6.0f);
        float thumbY = (float) y + ((float) height - thumbH) * 0.5f;
        g.fillRoundedRectangle ((float) thumbStart, thumbY,
                                (float) thumbSize, thumbH, thumbH * 0.5f);
    }
}

int ChordyLookAndFeel::getDefaultScrollbarWidth()
{
    return 8;
}
