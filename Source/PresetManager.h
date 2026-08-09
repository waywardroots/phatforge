#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <utility>

// Forward declaration only - PresetManager.cpp includes the full processor
// header, keeping this header lightweight and avoiding a circular include.
class PhatForgeAudioProcessor;

// Handles everything to do with presets: saving the current APVTS state to
// disk, loading it back, stepping through the list, and seeding a handful
// of factory presets the first time the plugin ever runs on a machine.
//
// Presets are plain XML files (the same format used for DAW session state)
// stored one-per-file in a "PhatForge/Presets" folder inside the user's
// application data directory, so they show up next to where most other
// plugins keep their presets.
class PresetManager : public juce::ChangeBroadcaster
{
public:
    static const juce::String presetFileExtension;

    explicit PresetManager(PhatForgeAudioProcessor& processorToUse);

    void savePreset(const juce::String& name);
    void deletePreset(const juce::String& name);
    void loadPreset(const juce::String& name);
    void loadNextPreset();
    void loadPreviousPreset();
    void loadInitPreset();

    juce::StringArray getAllPresetNames() const;
    juce::String getCurrentPresetName() const { return currentPresetName; }

    // Called by the processor's setStateInformation() so the preset bar in
    // the GUI reflects the right name after a DAW session reload, without
    // re-triggering a disk load (the parameter values already came from the
    // session's own saved state).
    void setCurrentPresetNameSilently(const juce::String& name)
    {
        currentPresetName = name;
        sendChangeMessage();
    }

    static juce::File getPresetsFolder();

private:
    void resetAllParametersToDefault();
    void applyOverrides(const std::vector<std::pair<juce::String, float>>& overrides);
    void writeFactoryPreset(const juce::String& name, const std::vector<std::pair<juce::String, float>>& overrides);
    void createFactoryPresetsIfNeeded();

    PhatForgeAudioProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;
    juce::String currentPresetName { "Init" };

    JUCE_DECLARE_NON_COPYABLE(PresetManager)
};
