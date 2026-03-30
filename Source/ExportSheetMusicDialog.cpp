#include "ExportSheetMusicDialog.h"
#include "ChordDetector.h"

ExportSheetMusicDialog::ExportSheetMusicDialog (ContentType type,
                                                const juce::String& defaultTitle,
                                                int originalKeyPitchClass)
    : contentType (type),
      originalKey (originalKeyPitchClass)
{
    // Title
    addAndMakeVisible (titleLabel);
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (ChordyTheme::textPrimary));

    addAndMakeVisible (titleEditor);
    titleEditor.setText (defaultTitle);
    titleEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour (ChordyTheme::bgElevated));
    titleEditor.setColour (juce::TextEditor::textColourId, juce::Colour (ChordyTheme::textPrimary));
    titleEditor.setColour (juce::TextEditor::outlineColourId, juce::Colour (ChordyTheme::border));

    // Composer
    addAndMakeVisible (composerLabel);
    composerLabel.setColour (juce::Label::textColourId, juce::Colour (ChordyTheme::textPrimary));

    addAndMakeVisible (composerEditor);
    composerEditor.setColour (juce::TextEditor::backgroundColourId, juce::Colour (ChordyTheme::bgElevated));
    composerEditor.setColour (juce::TextEditor::textColourId, juce::Colour (ChordyTheme::textPrimary));
    composerEditor.setColour (juce::TextEditor::outlineColourId, juce::Colour (ChordyTheme::border));

    // Key checkboxes
    addAndMakeVisible (keysLabel);
    keysLabel.setColour (juce::Label::textColourId, juce::Colour (ChordyTheme::textPrimary));
    keysLabel.setFont (juce::Font (ChordyTheme::fontSectionHead));

    for (int i = 0; i < 12; ++i)
    {
        keyButtons[i].setButtonText (ChordDetector::noteNameFromPitchClass (i));
        keyButtons[i].setToggleState (i == originalKey, juce::dontSendNotification);
        keyButtons[i].setColour (juce::ToggleButton::textColourId, juce::Colour (ChordyTheme::textPrimary));
        keyButtons[i].setColour (juce::ToggleButton::tickColourId, juce::Colour (ChordyTheme::accent));
        addAndMakeVisible (keyButtons[i]);
    }

    // Key selection buttons
    addAndMakeVisible (selectAllBtn);
    selectAllBtn.onClick = [this]
    {
        for (auto& kb : keyButtons)
            kb.setToggleState (true, juce::dontSendNotification);
    };

    addAndMakeVisible (selectNoneBtn);
    selectNoneBtn.onClick = [this]
    {
        for (auto& kb : keyButtons)
            kb.setToggleState (false, juce::dontSendNotification);
    };

    addAndMakeVisible (originalOnlyBtn);
    originalOnlyBtn.onClick = [this]
    {
        for (int i = 0; i < 12; ++i)
            keyButtons[i].setToggleState (i == originalKey, juce::dontSendNotification);
    };

    // Options
    addAndMakeVisible (optionsLabel);
    optionsLabel.setColour (juce::Label::textColourId, juce::Colour (ChordyTheme::textPrimary));
    optionsLabel.setFont (juce::Font (ChordyTheme::fontSectionHead));

    addAndMakeVisible (chordSymbolsToggle);
    chordSymbolsToggle.setToggleState (true, juce::dontSendNotification);
    chordSymbolsToggle.setColour (juce::ToggleButton::textColourId, juce::Colour (ChordyTheme::textPrimary));
    chordSymbolsToggle.setColour (juce::ToggleButton::tickColourId, juce::Colour (ChordyTheme::accent));

    addAndMakeVisible (grandStaffToggle);
    grandStaffToggle.setToggleState (true, juce::dontSendNotification);
    grandStaffToggle.setColour (juce::ToggleButton::textColourId, juce::Colour (ChordyTheme::textPrimary));
    grandStaffToggle.setColour (juce::ToggleButton::tickColourId, juce::Colour (ChordyTheme::accent));
    // Hide grand staff for melody (always single staff)
    grandStaffToggle.setVisible (type != ContentType::Melody);

    // Paper size
    addAndMakeVisible (paperLabel);
    paperLabel.setColour (juce::Label::textColourId, juce::Colour (ChordyTheme::textPrimary));

    addAndMakeVisible (paperSizeCombo);
    paperSizeCombo.addItem ("Letter", 1);
    paperSizeCombo.addItem ("A4", 2);
    paperSizeCombo.setSelectedId (1, juce::dontSendNotification);
    paperSizeCombo.setColour (juce::ComboBox::backgroundColourId, juce::Colour (ChordyTheme::bgElevated));
    paperSizeCombo.setColour (juce::ComboBox::textColourId, juce::Colour (ChordyTheme::textPrimary));
    paperSizeCombo.setColour (juce::ComboBox::outlineColourId, juce::Colour (ChordyTheme::border));

    // Status
    addAndMakeVisible (statusLabel);
    statusLabel.setFont (juce::Font (ChordyTheme::fontSmall));

    // Buttons
    addAndMakeVisible (cancelBtn);
    cancelBtn.onClick = [this]
    {
        if (onCancel) onCancel();
    };

    addAndMakeVisible (exportBtn);
    exportBtn.setColour (juce::TextButton::buttonColourId, juce::Colour (ChordyTheme::accent));
    exportBtn.setColour (juce::TextButton::textColourOnId, juce::Colour (0xFF000000));
    exportBtn.setColour (juce::TextButton::textColourOffId, juce::Colour (0xFF000000));
    exportBtn.onClick = [this]
    {
        if (onExport) onExport();
    };

    updateLilyPondStatus();

    setSize (420, 520);
}

void ExportSheetMusicDialog::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (ChordyTheme::bgSurface));
}

void ExportSheetMusicDialog::resized()
{
    auto area = getLocalBounds().reduced (16);
    const int rowH = 28;
    const int gap = 6;

    // Title
    auto row = area.removeFromTop (rowH);
    titleLabel.setBounds (row.removeFromLeft (80));
    titleEditor.setBounds (row);
    area.removeFromTop (gap);

    // Composer
    row = area.removeFromTop (rowH);
    composerLabel.setBounds (row.removeFromLeft (80));
    composerEditor.setBounds (row);
    area.removeFromTop (gap + 4);

    // Keys label
    keysLabel.setBounds (area.removeFromTop (rowH));
    area.removeFromTop (2);

    // Key checkboxes: 4 columns x 3 rows
    int colW = area.getWidth() / 4;
    for (int r = 0; r < 3; ++r)
    {
        row = area.removeFromTop (24);
        for (int c = 0; c < 4; ++c)
        {
            int idx = r * 4 + c;
            if (idx < 12)
                keyButtons[idx].setBounds (row.removeFromLeft (colW));
        }
        area.removeFromTop (2);
    }

    area.removeFromTop (4);

    // Selection buttons
    row = area.removeFromTop (rowH);
    int btnW = (area.getWidth() - 8) / 3;
    selectAllBtn.setBounds (row.removeFromLeft (btnW));
    row.removeFromLeft (4);
    selectNoneBtn.setBounds (row.removeFromLeft (btnW));
    row.removeFromLeft (4);
    originalOnlyBtn.setBounds (row);
    area.removeFromTop (gap + 4);

    // Options label
    optionsLabel.setBounds (area.removeFromTop (rowH));
    area.removeFromTop (2);

    // Chord symbols toggle
    chordSymbolsToggle.setBounds (area.removeFromTop (24));
    area.removeFromTop (2);

    // Grand staff toggle
    if (grandStaffToggle.isVisible())
    {
        grandStaffToggle.setBounds (area.removeFromTop (24));
        area.removeFromTop (2);
    }

    // Paper size
    row = area.removeFromTop (rowH);
    paperLabel.setBounds (row.removeFromLeft (60));
    paperSizeCombo.setBounds (row.removeFromLeft (100));
    area.removeFromTop (gap + 4);

    // Status
    statusLabel.setBounds (area.removeFromTop (40));
    area.removeFromTop (gap);

    // Buttons at bottom
    row = area.removeFromBottom (34);
    exportBtn.setBounds (row.removeFromRight (140));
    row.removeFromRight (8);
    cancelBtn.setBounds (row.removeFromRight (100));
}

LilyPondExporter::ExportOptions ExportSheetMusicDialog::getOptions() const
{
    LilyPondExporter::ExportOptions opts;

    opts.title = titleEditor.getText().trim();
    opts.composer = composerEditor.getText().trim();

    for (int i = 0; i < 12; ++i)
        if (keyButtons[i].getToggleState())
            opts.keys.push_back (i);

    opts.includeChordSymbols = chordSymbolsToggle.getToggleState();
    opts.grandStaff = grandStaffToggle.isVisible() ? grandStaffToggle.getToggleState() : false;

    opts.paperSize = (paperSizeCombo.getSelectedId() == 2) ? "a4" : "letter";

    return opts;
}

void ExportSheetMusicDialog::updateLilyPondStatus()
{
    auto bin = LilyPondExporter::findLilyPondBinary();
    if (bin.existsAsFile())
    {
        statusLabel.setText ("LilyPond: " + bin.getFullPathName(),
                             juce::dontSendNotification);
        statusLabel.setColour (juce::Label::textColourId, juce::Colour (ChordyTheme::success));
        exportBtn.setEnabled (true);
    }
    else
    {
        statusLabel.setText ("LilyPond not found.\nInstall via: brew install lilypond",
                             juce::dontSendNotification);
        statusLabel.setColour (juce::Label::textColourId, juce::Colour (ChordyTheme::danger));
        exportBtn.setEnabled (false);
    }
}

juce::DialogWindow* ExportSheetMusicDialog::show (
    ContentType type,
    const juce::String& defaultTitle,
    int originalKeyPitchClass,
    std::function<void (LilyPondExporter::ExportOptions)> onExportCallback)
{
    auto* content = new ExportSheetMusicDialog (type, defaultTitle, originalKeyPitchClass);

    juce::DialogWindow::LaunchOptions launchOpts;
    launchOpts.dialogTitle = "Export Sheet Music";
    launchOpts.dialogBackgroundColour = juce::Colour (ChordyTheme::bgSurface);
    launchOpts.content.setOwned (content);
    launchOpts.useNativeTitleBar = true;
    launchOpts.resizable = false;

    auto* window = launchOpts.launchAsync();

    content->onExport = [content, window, onExportCallback]
    {
        auto opts = content->getOptions();
        if (opts.keys.empty())
        {
            juce::AlertWindow::showMessageBoxAsync (
                juce::MessageBoxIconType::WarningIcon,
                "No Keys Selected",
                "Please select at least one key to export.");
            return;
        }
        if (window != nullptr)
            window->exitModalState (0);
        if (onExportCallback)
            onExportCallback (opts);
    };

    content->onCancel = [window]
    {
        if (window != nullptr)
            window->exitModalState (0);
    };

    return window;
}
