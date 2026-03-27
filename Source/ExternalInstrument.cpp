#include "ExternalInstrument.h"

ExternalInstrument::ExternalInstrument()
{
}

ExternalInstrument::~ExternalInstrument()
{
    clearPlugin();
}

void ExternalInstrument::initialize()
{
    // Register plugin formats manually (addDefaultFormats is deleted in headless builds)
   #if JUCE_PLUGINHOST_VST3
    formatManager.addFormat (new juce::VST3PluginFormat());
   #endif
   #if JUCE_PLUGINHOST_AU
    formatManager.addFormat (new juce::AudioUnitPluginFormat());
   #endif
    loadPluginListCache();
}

void ExternalInstrument::prepare (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    // Pre-allocate scratch buffer for channel mismatch path
    pluginBuffer.setSize (2, samplesPerBlock);

    const juce::ScopedLock sl (pluginLock);
    if (hostedPlugin != nullptr)
        hostedPlugin->prepareToPlay (sampleRate, samplesPerBlock);
}

void ExternalInstrument::release()
{
    const juce::ScopedLock sl (pluginLock);
    if (hostedPlugin != nullptr)
        hostedPlugin->releaseResources();
}

bool ExternalInstrument::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    const juce::ScopedTryLock sl (pluginLock);
    if (! sl.isLocked() || hostedPlugin == nullptr)
        return false;

    int hostChannels = buffer.getNumChannels();
    int pluginChannels = hostedPluginNumChannels;

    if (pluginChannels <= 0 || pluginChannels == hostChannels)
    {
        // Fast path: direct passthrough (zero overhead)
        hostedPlugin->processBlock (buffer, midi);
    }
    else
    {
        // Channel mismatch: use scratch buffer
        int numSamples = buffer.getNumSamples();
        pluginBuffer.setSize (pluginChannels, numSamples, false, false, true);
        pluginBuffer.clear();

        int channelsToCopy = juce::jmin (hostChannels, pluginChannels);
        for (int ch = 0; ch < channelsToCopy; ++ch)
            pluginBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

        hostedPlugin->processBlock (pluginBuffer, midi);

        buffer.clear();
        channelsToCopy = juce::jmin (hostChannels, pluginChannels);
        for (int ch = 0; ch < channelsToCopy; ++ch)
            buffer.copyFrom (ch, 0, pluginBuffer, ch, 0, numSamples);

        // If plugin is mono but host is stereo, duplicate mono to both channels
        if (pluginChannels == 1 && hostChannels >= 2)
            buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
    }

    return true;
}

void ExternalInstrument::setupPluginBuses (juce::AudioPluginInstance& instance)
{
    // Try stereo in/out first (most common for instruments)
    auto stereoLayout = juce::AudioProcessor::BusesLayout();
    stereoLayout.inputBuses.add (juce::AudioChannelSet::stereo());
    stereoLayout.outputBuses.add (juce::AudioChannelSet::stereo());

    if (instance.setBusesLayout (stereoLayout))
    {
        hostedPluginNumChannels = instance.getTotalNumOutputChannels();
        return;
    }

    // Try stereo output only (no input bus -- common for synth instruments)
    auto stereoOutLayout = juce::AudioProcessor::BusesLayout();
    stereoOutLayout.outputBuses.add (juce::AudioChannelSet::stereo());

    if (instance.setBusesLayout (stereoOutLayout))
    {
        hostedPluginNumChannels = instance.getTotalNumOutputChannels();
        return;
    }

    // Accept the plugin's default layout
    hostedPluginNumChannels = instance.getTotalNumOutputChannels();
}

void ExternalInstrument::loadPlugin (const juce::PluginDescription& desc)
{
    clearPlugin();
    int myGeneration = ++loadGeneration;

    // Write loading pedal before attempting (crash protection for SIGSEGV)
    getLoadingPedalFile().replaceWithText (desc.name);

    formatManager.createPluginInstanceAsync (
        desc, currentSampleRate, currentBlockSize,
        [this, myGeneration] (std::unique_ptr<juce::AudioPluginInstance> instance,
                               const juce::String& error)
        {
            if (myGeneration != loadGeneration)
                return;  // stale load, a newer one was initiated

            if (instance == nullptr)
            {
                DBG ("Failed to load plugin: " + error);
                getLoadingPedalFile().deleteFile();
                if (onPluginLoadFailed)
                    onPluginLoadFailed (error);
                return;
            }

            try
            {
                setupPluginBuses (*instance);
                instance->setRateAndBufferSizeDetails (currentSampleRate, currentBlockSize);
                instance->prepareToPlay (currentSampleRate, currentBlockSize);
            }
            catch (...)
            {
                DBG ("Plugin threw an exception during prepareToPlay");
                getLoadingPedalFile().deleteFile();
                if (onPluginLoadFailed)
                    onPluginLoadFailed ("Plugin crashed during preparation");
                return;
            }

            {
                const juce::ScopedLock sl (pluginLock);
                hostedPlugin = std::move (instance);
            }

            getLoadingPedalFile().deleteFile();
            DBG ("Loaded plugin: " + hostedPlugin->getName());

            if (onPluginLoaded)
                onPluginLoaded();
        });
}

void ExternalInstrument::clearPlugin()
{
    const juce::ScopedLock sl (pluginLock);
    if (hostedPlugin != nullptr)
    {
        hostedPlugin->releaseResources();
        hostedPlugin.reset();
    }
    hostedPluginNumChannels = 0;
}

bool ExternalInstrument::isPluginLoaded() const
{
    return hostedPlugin != nullptr;
}

juce::String ExternalInstrument::getPluginName() const
{
    if (hostedPlugin != nullptr)
        return hostedPlugin->getName();
    return {};
}

juce::Array<juce::PluginDescription> ExternalInstrument::getAvailableInstruments() const
{
    juce::Array<juce::PluginDescription> instruments;
    juce::StringArray seenNames;

    for (const auto& desc : pluginList.getTypes())
    {
        if (! desc.isInstrument)
            continue;

        // Deduplicate by name -- prefer VST3 over AU
        int existingIdx = seenNames.indexOf (desc.name);
        if (existingIdx >= 0)
        {
            // Replace AU with VST3 if we find the VST3 version
            if (desc.pluginFormatName == "VST3")
                instruments.set (existingIdx, desc);
            continue;
        }

        seenNames.add (desc.name);
        instruments.add (desc);
    }

    return instruments;
}

void ExternalInstrument::startScan()
{
    if (scanning)
        return;

    scanning = true;

    auto deadMansPedal = getDeadMansPedalFile();
    auto* self = this;

    juce::Thread::launch ([self, deadMansPedal]
    {
        for (int i = 0; i < self->formatManager.getNumFormats(); ++i)
        {
            auto* format = self->formatManager.getFormat (i);
            auto searchPaths = format->getDefaultLocationsToSearch();

            juce::PluginDirectoryScanner scanner (
                self->pluginList, *format,
                searchPaths,
                true, // recursive
                deadMansPedal);

            juce::String pluginName;
            while (scanner.scanNextFile (true, pluginName))
            {
                DBG ("Scanning: " + pluginName);
            }
        }

        self->scanning = false;
        self->savePluginListCache();

        juce::MessageManager::callAsync ([self]
        {
            if (self->onPluginListChanged)
                self->onPluginListChanged();
        });
    });
}

//==============================================================================
// State persistence
//==============================================================================

std::unique_ptr<juce::XmlElement> ExternalInstrument::createStateXml() const
{
    auto xml = std::make_unique<juce::XmlElement> ("ExternalInstrument");

    if (hostedPlugin != nullptr)
    {
        // Save plugin description
        auto desc = hostedPlugin->getPluginDescription();
        xml->addChildElement (desc.createXml().release());

        // Save plugin state as base64
        juce::MemoryBlock stateData;
        hostedPlugin->getStateInformation (stateData);
        xml->setAttribute ("pluginState", stateData.toBase64Encoding());
    }

    return xml;
}

void ExternalInstrument::restoreFromStateXml (const juce::XmlElement& xml,
                                               double sampleRate, int blockSize)
{
    currentSampleRate = sampleRate;
    currentBlockSize = blockSize;

    auto* descXml = xml.getChildElement (0);
    if (descXml == nullptr)
        return;

    juce::PluginDescription desc;
    if (! desc.loadFromXml (*descXml))
        return;

    // Check loading dead-mans-pedal: if this plugin crashed last time, skip it
    auto loadingPedal = getLoadingPedalFile();
    if (loadingPedal.existsAsFile())
    {
        auto crashedPlugin = loadingPedal.loadFileAsString().trim();
        if (crashedPlugin == desc.name)
        {
            DBG ("Skipping plugin that previously crashed during loading: " + desc.name);
            loadingPedal.deleteFile();
            if (onPluginLoadFailed)
                onPluginLoadFailed ("Plugin '" + desc.name + "' previously crashed and was blocked. Select it again to retry.");
            return;
        }
        // Different plugin -- previous crash was from something else, clear it
        loadingPedal.deleteFile();
    }

    // Save pending state data for application after async load
    juce::MemoryBlock savedState;
    auto stateBase64 = xml.getStringAttribute ("pluginState");
    if (stateBase64.isNotEmpty())
        savedState.fromBase64Encoding (stateBase64);

    int myGeneration = ++loadGeneration;

    // Write loading pedal before attempting (crash protection for SIGSEGV)
    loadingPedal.replaceWithText (desc.name);

    formatManager.createPluginInstanceAsync (
        desc, currentSampleRate, currentBlockSize,
        [this, myGeneration, pendingState = std::move (savedState)]
        (std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& error)
        {
            if (myGeneration != loadGeneration)
                return;  // stale load

            if (instance == nullptr)
            {
                DBG ("Failed to restore plugin: " + error);
                getLoadingPedalFile().deleteFile();
                if (onPluginLoadFailed)
                    onPluginLoadFailed (error);
                return;
            }

            try
            {
                // Restore plugin state before prepareToPlay
                if (! pendingState.isEmpty())
                    instance->setStateInformation (pendingState.getData(),
                                                    static_cast<int> (pendingState.getSize()));

                setupPluginBuses (*instance);
                instance->setRateAndBufferSizeDetails (currentSampleRate, currentBlockSize);
                instance->prepareToPlay (currentSampleRate, currentBlockSize);
            }
            catch (...)
            {
                DBG ("Plugin threw during restore/prepare");
                getLoadingPedalFile().deleteFile();
                if (onPluginLoadFailed)
                    onPluginLoadFailed ("Plugin crashed during state restoration");
                return;
            }

            {
                const juce::ScopedLock sl (pluginLock);
                hostedPlugin = std::move (instance);
            }

            getLoadingPedalFile().deleteFile();
            DBG ("Restored plugin: " + hostedPlugin->getName());

            if (onPluginLoaded)
                onPluginLoaded();
        });
}

//==============================================================================
// Plugin list cache
//==============================================================================

juce::File ExternalInstrument::getPluginListCacheFile() const
{
    auto appDataDir = juce::File::getSpecialLocation (
        juce::File::userApplicationDataDirectory)
        .getChildFile ("Chordy");
    appDataDir.createDirectory();
    return appDataDir.getChildFile ("pluginList.xml");
}

juce::File ExternalInstrument::getDeadMansPedalFile() const
{
    auto appDataDir = juce::File::getSpecialLocation (
        juce::File::userApplicationDataDirectory)
        .getChildFile ("Chordy");
    appDataDir.createDirectory();
    return appDataDir.getChildFile ("deadMansPedal.txt");
}

juce::File ExternalInstrument::getLoadingPedalFile() const
{
    auto appDataDir = juce::File::getSpecialLocation (
        juce::File::userApplicationDataDirectory)
        .getChildFile ("Chordy");
    appDataDir.createDirectory();
    return appDataDir.getChildFile ("loadingPlugin.txt");
}

void ExternalInstrument::loadPluginListCache()
{
    auto file = getPluginListCacheFile();
    if (file.existsAsFile())
    {
        auto xml = juce::XmlDocument::parse (file);
        if (xml != nullptr)
            pluginList.recreateFromXml (*xml);
    }
}

void ExternalInstrument::savePluginListCache()
{
    auto xml = pluginList.createXml();
    if (xml != nullptr)
    {
        auto file = getPluginListCacheFile();
        xml->writeTo (file);
    }
}
