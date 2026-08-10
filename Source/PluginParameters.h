#pragma once
#include <memory>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>

// Central place for every parameter ID string used by the plugin, plus the
// function that builds the APVTS parameter layout. Keeping this in one file
// means the processor and editor can never disagree about an ID.

namespace ID
{
    // Global
    static const juce::String inputGain   = "inputGain";
    static const juce::String outputGain  = "outputGain";
    static const juce::String mix         = "mix";

    // Filter section (3 resonant filters in series)
    static const juce::String filterType[3]  = { "filter1Type",  "filter2Type",  "filter3Type" };
    static const juce::String filterFreq[3]  = { "filter1Freq",  "filter2Freq",  "filter3Freq" };
    static const juce::String filterRes[3]   = { "filter1Res",   "filter2Res",   "filter3Res" };
    static const juce::String filterOn[3]    = { "filter1On",    "filter2On",    "filter3On" };

    // Distortion
    static const juce::String distType   = "distType";
    static const juce::String distDrive  = "distDrive";
    static const juce::String distTone   = "distTone";
    static const juce::String distMix    = "distMix";

    // Magic EQ (low end enhancer)
    static const juce::String eqFreq     = "eqFreq";
    static const juce::String eqAmount   = "eqAmount";

    // Compressor
    static const juce::String compThreshold = "compThreshold";
    static const juce::String compRatio     = "compRatio";
    static const juce::String compAttack    = "compAttack";
    static const juce::String compRelease   = "compRelease";
    static const juce::String compMakeup    = "compMakeup";

    // LFOs
    static const juce::String lfoRate[2]   = { "lfo1Rate",   "lfo2Rate" };
    static const juce::String lfoDepth[2]  = { "lfo1Depth",  "lfo2Depth" };
    static const juce::String lfoWave[2]   = { "lfo1Wave",   "lfo2Wave" };
    static const juce::String lfoTarget[2] = { "lfo1Target", "lfo2Target" };

    // Envelope follower
    static const juce::String envAmount  = "envAmount";
    static const juce::String envAttack  = "envAttack";
    static const juce::String envRelease = "envRelease";
    static const juce::String envTarget  = "envTarget";
}

// Modulation targets shared by the LFOs and the envelope follower.
enum class ModTarget
{
    None = 0,
    Filter1Freq,
    Filter2Freq,
    Filter3Freq,
    DistDrive,
    EqAmount,
    CompThreshold,
    NumTargets
};

inline juce::StringArray modTargetChoices()
{
    return { "Off", "Filter 1 Freq", "Filter 2 Freq", "Filter 3 Freq",
             "Dist Drive", "Magic EQ Amount", "Comp Threshold" };
}

inline juce::StringArray filterTypeChoices()
{
    return { "Low Pass", "High Pass", "Band Pass", "Notch", "Peak/Bell", "Low Shelf", "High Shelf", "Comb" };
}
inline juce::StringArray distTypeChoices()
{
    return { "Tube", "Crush", "Exciter", "Mech", "Diode", "Fold", "Rectify", "Shaper" };
}
inline juce::StringArray lfoWaveChoices()     { return { "Sine", "Triangle", "Square", "Saw" }; }

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto dbRange   = juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f);
    auto freqRange = juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f);

    params.push_back(std::make_unique<juce::AudioParameterFloat>(ID::inputGain,  "Input Gain",  dbRange, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(ID::outputGain, "Output Gain", dbRange, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(ID::mix,        "Dry/Wet",     juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

    for (int i = 0; i < 3; ++i)
    {
        params.push_back(std::make_unique<juce::AudioParameterChoice>(ID::filterType[i], "Filter " + juce::String(i + 1) + " Type", filterTypeChoices(), 0));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(ID::filterFreq[i], "Filter " + juce::String(i + 1) + " Freq", freqRange, 2000.0f + i * 1000.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(ID::filterRes[i],  "Filter " + juce::String(i + 1) + " Res",  juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.4f), 0.7f));
        params.push_back(std::make_unique<juce::AudioParameterBool>(ID::filterOn[i],   "Filter " + juce::String(i + 1) + " On",   false));
    }

    params.push_back(std::make_unique<juce::AudioParameterChoice>(ID::distType, "Distortion Type", distTypeChoices(), 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(ID::distDrive, "Drive", juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(ID::distTone,  "Tone",  juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(ID::distMix,   "Dist Mix", juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(ID::eqFreq,   "Magic EQ Freq", juce::NormalisableRange<float>(40.0f, 500.0f, 1.0f, 0.5f), 120.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(ID::eqAmount, "Magic EQ Amount", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(ID::compThreshold, "Comp Threshold", juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -18.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(ID::compRatio,     "Comp Ratio",     juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f, 0.5f), 3.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(ID::compAttack,    "Comp Attack",    juce::NormalisableRange<float>(0.1f, 200.0f, 0.1f, 0.4f), 10.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(ID::compRelease,   "Comp Release",   juce::NormalisableRange<float>(5.0f, 1000.0f, 1.0f, 0.4f), 120.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(ID::compMakeup,    "Comp Makeup",    dbRange, 0.0f));

    for (int i = 0; i < 2; ++i)
    {
        params.push_back(std::make_unique<juce::AudioParameterFloat>(ID::lfoRate[i], "LFO " + juce::String(i + 1) + " Rate", juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.4f), 1.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(ID::lfoDepth[i], "LFO " + juce::String(i + 1) + " Depth", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(ID::lfoWave[i], "LFO " + juce::String(i + 1) + " Wave", lfoWaveChoices(), 0));
        params.push_back(std::make_unique<juce::AudioParameterChoice>(ID::lfoTarget[i], "LFO " + juce::String(i + 1) + " Target", modTargetChoices(), 0));
    }

    params.push_back(std::make_unique<juce::AudioParameterFloat>(ID::envAmount, "Env Amount", juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(ID::envAttack, "Env Attack", juce::NormalisableRange<float>(0.1f, 200.0f, 0.1f, 0.4f), 5.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(ID::envRelease,"Env Release", juce::NormalisableRange<float>(5.0f, 1000.0f, 1.0f, 0.4f), 150.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(ID::envTarget, "Env Target", modTargetChoices(), 0));

    return { params.begin(), params.end() };
}
