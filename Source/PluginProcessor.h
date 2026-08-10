#pragma once
#include <array>
#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "PluginParameters.h"
#include "DSP/Distortion.h"
#include "DSP/MagicEQ.h"
#include "DSP/FilterSection.h"
#include "DSP/ModLFO.h"
#include "DSP/EnvelopeFollower.h"
#include "PresetManager.h"

class PhatForgeAudioProcessor : public juce::AudioProcessor
{
public:
    PhatForgeAudioProcessor();
    ~PhatForgeAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "PhatForge"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void randomiseParameters();

    PresetManager& getPresetManager() { return *presetManager; }

    // Metering / visualisation getters used by the custom GUI. Thread-safe,
    // lock-free - just atomics updated once per block.
    float getOutputLevel(int channel) const { return channel == 0 ? outputLevelL.load() : outputLevelR.load(); }
    float getModValueForMeter(ModTarget t) const { return modValues[(size_t) t]; }
    float getEnvelopeValue() const { return lastEnvValue; }

    juce::AudioProcessorValueTreeState apvts;

private:
    std::unique_ptr<PresetManager> presetManager;

    Distortion distortion;
    MagicEQ magicEq;
    std::array<PhatFilter, 3> filters;
    juce::dsp::Compressor<float> compressor;
    juce::dsp::Gain<float> inputGainProc, outputGainProc, makeupGainProc;

    std::array<ModLFO, 2> lfos;
    EnvelopeFollower envFollower;

    // Latest computed modulation contribution per target, refreshed once per block.
    std::array<float, (size_t) ModTarget::NumTargets> modValues {};
    float lastEnvValue = 0.0f;

    std::atomic<float> outputLevelL { 0.0f }, outputLevelR { 0.0f };

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhatForgeAudioProcessor)
};
