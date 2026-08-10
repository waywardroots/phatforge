#include "PresetManager.h"
#include "PluginProcessor.h"

const juce::String PresetManager::presetFileExtension = ".phatpreset";

PresetManager::PresetManager(PhatForgeAudioProcessor& processorToUse)
    : processor(processorToUse), apvts(processorToUse.apvts)
{
    createFactoryPresetsIfNeeded();
}

juce::File PresetManager::getPresetsFolder()
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("PhatForge")
                   .getChildFile("Presets");

    if (! dir.isDirectory())
        dir.createDirectory();

    return dir;
}

void PresetManager::resetAllParametersToDefault()
{
    for (auto* p : processor.getParameters())
        p->setValueNotifyingHost(p->getDefaultValue());
}

void PresetManager::applyOverrides(const std::vector<std::pair<juce::String, float>>& overrides)
{
    for (auto& kv : overrides)
        if (auto* param = apvts.getParameter(kv.first))
            param->setValueNotifyingHost(param->convertTo0to1(kv.second));
}

void PresetManager::savePreset(const juce::String& name)
{
    if (name.isEmpty())
        return;

    auto file = getPresetsFolder().getChildFile(juce::File::createLegalFileName(name) + presetFileExtension);

    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    xml->setAttribute("presetName", name);
    file.replaceWithText(xml->toString());

    currentPresetName = name;
    sendChangeMessage();
}

void PresetManager::deletePreset(const juce::String& name)
{
    auto file = getPresetsFolder().getChildFile(juce::File::createLegalFileName(name) + presetFileExtension);
    if (file.existsAsFile())
        file.deleteFile();

    sendChangeMessage();
}

void PresetManager::loadPreset(const juce::String& name)
{
    auto file = getPresetsFolder().getChildFile(juce::File::createLegalFileName(name) + presetFileExtension);
    if (! file.existsAsFile())
        return;

    if (auto xml = juce::XmlDocument::parse(file))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
        currentPresetName = name;
        sendChangeMessage();
    }
}

void PresetManager::loadInitPreset()
{
    resetAllParametersToDefault();
    currentPresetName = "Init";
    sendChangeMessage();
}

juce::StringArray PresetManager::getAllPresetNames() const
{
    juce::StringArray names;

    for (const auto& f : getPresetsFolder().findChildFiles(juce::File::findFiles, false, "*" + presetFileExtension))
        names.add(f.getFileNameWithoutExtension());

    names.sort(true);
    return names;
}

void PresetManager::loadNextPreset()
{
    auto names = getAllPresetNames();
    if (names.isEmpty())
        return;

    auto idx = names.indexOf(currentPresetName);
    idx = (idx + 1) % names.size();
    loadPreset(names[idx]);
}

void PresetManager::loadPreviousPreset()
{
    auto names = getAllPresetNames();
    if (names.isEmpty())
        return;

    auto idx = names.indexOf(currentPresetName);
    idx = (idx <= 0) ? names.size() - 1 : idx - 1;
    loadPreset(names[idx]);
}

void PresetManager::writeFactoryPreset(const juce::String& name, const std::vector<std::pair<juce::String, float>>& overrides)
{
    auto file = getPresetsFolder().getChildFile(juce::File::createLegalFileName(name) + presetFileExtension);
    if (file.existsAsFile())
        return; // never clobber a preset the user already has (incl. their own edited "factory" preset)

    resetAllParametersToDefault();
    applyOverrides(overrides);

    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    xml->setAttribute("presetName", name);
    file.replaceWithText(xml->toString());
}

void PresetManager::createFactoryPresetsIfNeeded()
{
    using Overrides = std::vector<std::pair<juce::String, float>>;

    // A handful of starting points that show off the different distortion
    // and filter characters. Only ever written once per machine - if the
    // user deletes or edits one, it won't be silently recreated.
    writeFactoryPreset("Init", {});

    writeFactoryPreset("Warm Bass Glue", Overrides {
        { "filter1On", 1.0f }, { "filter1Type", 5.0f }, { "filter1Freq", 100.0f }, { "filter1Res", 7.0f },
        { "distType", 0.0f }, { "distDrive", 0.4f }, { "distTone", 0.6f }, { "distMix", 0.5f },
        { "eqFreq", 90.0f }, { "eqAmount", 0.6f },
        { "compThreshold", -20.0f }, { "compRatio", 4.0f }, { "compAttack", 15.0f },
        { "compRelease", 140.0f }, { "compMakeup", 3.0f },
    });

    writeFactoryPreset("Crunch Drums", Overrides {
        { "distType", 3.0f }, { "distDrive", 0.7f }, { "distTone", 0.4f }, { "distMix", 0.8f },
        { "filter1On", 1.0f }, { "filter1Type", 4.0f }, { "filter1Freq", 3000.0f }, { "filter1Res", 6.0f },
        { "compThreshold", -12.0f }, { "compRatio", 6.0f }, { "compAttack", 3.0f },
        { "compRelease", 80.0f }, { "compMakeup", 4.0f },
    });

    writeFactoryPreset("Robo Comb Riser", Overrides {
        { "filter1On", 1.0f }, { "filter1Type", 7.0f }, { "filter1Freq", 800.0f }, { "filter1Res", 8.0f },
        { "lfo1Rate", 2.0f }, { "lfo1Depth", 0.6f }, { "lfo1Wave", 1.0f }, { "lfo1Target", 1.0f },
        { "mix", 1.0f },
    });

    writeFactoryPreset("Sub Enhancer", Overrides {
        { "eqFreq", 70.0f }, { "eqAmount", 0.8f },
        { "filter1On", 1.0f }, { "filter1Type", 0.0f }, { "filter1Freq", 150.0f }, { "filter1Res", 2.0f },
        { "outputGain", 1.0f },
    });

    writeFactoryPreset("Diode Grind Guitar", Overrides {
        { "distType", 4.0f }, { "distDrive", 0.55f }, { "distTone", 0.45f }, { "distMix", 0.7f },
        { "filter2On", 1.0f }, { "filter2Type", 1.0f }, { "filter2Freq", 180.0f }, { "filter2Res", 1.0f },
        { "compThreshold", -16.0f }, { "compRatio", 3.0f },
    });

    // Leave the live plugin parameters at their defaults after seeding the
    // disk presets - the loop above briefly changes them for each preset it
    // writes, but nothing has been passed to the audio callback yet at this
    // point in construction, so there's no audible glitch.
    resetAllParametersToDefault();
    currentPresetName = "Init";
}
