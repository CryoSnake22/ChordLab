#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_data_structures/juce_data_structures.h>

class ExternalInstrument
{
public:
    ExternalInstrument();
    ~ExternalInstrument();

    void initialize();
    void prepare (double sampleRate, int samplesPerBlock);
    void release();

    // Process audio through the hosted plugin (audio thread).
    // Returns true if audio was rendered, false if no plugin loaded.
    bool processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);

    // Load a plugin by description (async, message thread)
    void loadPlugin (const juce::PluginDescription& desc);

    // Unload current plugin
    void clearPlugin();

    bool isPluginLoaded() const;
    juce::String getPluginName() const;

    // Get list of available instrument plugins (filtered from scan results)
    juce::Array<juce::PluginDescription> getAvailableInstruments() const;

    // Scan for plugins (runs on background thread)
    void startScan();
    bool isScanning() const { return scanning; }

    // State persistence
    std::unique_ptr<juce::XmlElement> createStateXml() const;
    void restoreFromStateXml (const juce::XmlElement& xml, double sampleRate, int blockSize);

    // Callbacks for UI refresh
    std::function<void()> onPluginListChanged;
    std::function<void()> onPluginLoaded;
    std::function<void (const juce::String& error)> onPluginLoadFailed;

    juce::KnownPluginList& getPluginList() { return pluginList; }

    // Get the hosted plugin instance (for creating its editor)
    juce::AudioPluginInstance* getHostedPlugin() const { return hostedPlugin.get(); }

private:
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList pluginList;

    juce::SpinLock pluginLock;
    std::unique_ptr<juce::AudioPluginInstance> hostedPlugin;

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    bool scanning = false;

    juce::File getPluginListCacheFile() const;
    juce::File getDeadMansPedalFile() const;
    void loadPluginListCache();
    void savePluginListCache();
};
