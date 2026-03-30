#pragma once

#include "LilyPondExporter.h"
#include "ChordyTheme.h"
#include <juce_gui_basics/juce_gui_basics.h>

class ExportSheetMusicDialog : public juce::Component
{
public:
    enum class ContentType { Voicing, Progression, Melody };

    ExportSheetMusicDialog (ContentType type,
                            const juce::String& defaultTitle,
                            int originalKeyPitchClass);

    void resized() override;
    void paint (juce::Graphics& g) override;

    LilyPondExporter::ExportOptions getOptions() const;

    std::function<void()> onExport;
    std::function<void()> onCancel;

    // Show the dialog in a window. Returns the window so caller can keep it alive.
    static juce::DialogWindow* show (ContentType type,
                                     const juce::String& defaultTitle,
                                     int originalKeyPitchClass,
                                     std::function<void (LilyPondExporter::ExportOptions)> onExportCallback);

private:
    ContentType contentType;
    int originalKey;

    juce::Label titleLabel { {}, "Title:" };
    juce::TextEditor titleEditor;
    juce::Label composerLabel { {}, "Composer:" };
    juce::TextEditor composerEditor;

    juce::Label keysLabel { {}, "Keys to export:" };
    juce::ToggleButton keyButtons[12];

    juce::TextButton selectAllBtn { "Select All" };
    juce::TextButton selectNoneBtn { "Select None" };
    juce::TextButton originalOnlyBtn { "Original Only" };

    juce::Label optionsLabel { {}, "Options:" };
    juce::ToggleButton chordSymbolsToggle { "Include chord symbols" };
    juce::ToggleButton grandStaffToggle { "Grand staff (piano)" };

    juce::Label paperLabel { {}, "Paper:" };
    juce::ComboBox paperSizeCombo;

    juce::Label statusLabel;

    juce::TextButton cancelBtn { "Cancel" };
    juce::TextButton exportBtn { "Export PDF..." };

    void updateLilyPondStatus();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExportSheetMusicDialog)
};
