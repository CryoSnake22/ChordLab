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

    juce::SpinLock::ScopedLockType lock (pluginLock);
    if (hostedPlugin != nullptr)
        hostedPlugin->prepareToPlay (sampleRate, samplesPerBlock);
}

void ExternalInstrument::release()
{
    juce::SpinLock::ScopedLockType lock (pluginLock);
    if (hostedPlugin != nullptr)
        hostedPlugin->releaseResources();
}

bool ExternalInstrument::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::SpinLock::ScopedTryLockType lock (pluginLock);
    if (! lock.isLocked() || hostedPlugin == nullptr)
        return false;

    hostedPlugin->processBlock (buffer, midi);
    return true;
}

void ExternalInstrument::loadPlugin (const juce::PluginDescription& desc)
{
    // Clear any existing plugin first to free resources
    clearPlugin();

    auto sr = currentSampleRate;
    auto bs = currentBlockSize;
    auto* self = this;
    auto descCopy = desc;

    // Load on a background thread to avoid blocking the message thread.
    // Some heavy plugins (Keyscape, Kontakt) do significant work during init.
    juce::Thread::launch ([self, descCopy, sr, bs]
    {
        juce::String errorMessage;
        std::unique_ptr<juce::AudioPluginInstance> instance;

        try
        {
            instance = self->formatManager.createPluginInstance (
                descCopy, sr, bs, errorMessage);
        }
        catch (...)
        {
            errorMessage = "Plugin threw an exception during loading";
        }

        if (instance == nullptr)
        {
            DBG ("Failed to load plugin: " + errorMessage);
            juce::MessageManager::callAsync ([self, errorMessage]
            {
                if (self->onPluginLoadFailed)
                    self->onPluginLoadFailed (errorMessage);
            });
            return;
        }

        try
        {
            instance->prepareToPlay (sr, bs);
        }
        catch (...)
        {
            DBG ("Plugin threw an exception during prepareToPlay");
            juce::MessageManager::callAsync ([self]
            {
                if (self->onPluginLoadFailed)
                    self->onPluginLoadFailed ("Plugin crashed during preparation");
            });
            return;
        }

        // Transfer ownership to message thread via raw pointer
        auto* rawInstance = instance.release();

        juce::MessageManager::callAsync ([self, rawInstance]
        {
            {
                juce::SpinLock::ScopedLockType lock (self->pluginLock);
                self->hostedPlugin.reset (rawInstance);
            }

            DBG ("Loaded plugin: " + self->hostedPlugin->getName());

            if (self->onPluginLoaded)
                self->onPluginLoaded();
        });
    });
}

void ExternalInstrument::clearPlugin()
{
    juce::SpinLock::ScopedLockType lock (pluginLock);
    if (hostedPlugin != nullptr)
    {
        hostedPlugin->releaseResources();
        hostedPlugin.reset();
    }
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

        // Deduplicate by name — prefer VST3 over AU
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

    // Load the plugin
    juce::String errorMessage;
    auto instance = formatManager.createPluginInstance (
        desc, currentSampleRate, currentBlockSize, errorMessage);

    if (instance == nullptr)
    {
        DBG ("Failed to restore plugin: " + errorMessage);
        return;
    }

    // Restore plugin state
    auto stateBase64 = xml.getStringAttribute ("pluginState");
    if (stateBase64.isNotEmpty())
    {
        juce::MemoryBlock stateData;
        stateData.fromBase64Encoding (stateBase64);
        instance->setStateInformation (stateData.getData(),
                                        static_cast<int> (stateData.getSize()));
    }

    instance->prepareToPlay (currentSampleRate, currentBlockSize);

    {
        juce::SpinLock::ScopedLockType lock (pluginLock);
        hostedPlugin = std::move (instance);
    }

    if (onPluginLoaded)
        onPluginLoaded();
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

