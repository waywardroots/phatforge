#include "PluginProcessor.h"
#include "PluginEditor.h"

PhatForgeAudioProcessor::PhatForgeAudioProcessor()
    : AudioProcessor(BusesProperties()
                          .withInput("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    presetManager = std::make_unique<PresetManager>(*this);
}

void PhatForgeAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock,
                                   (juce::uint32) getTotalNumOutputChannels() };

    inputGainProc.prepare(spec);
    outputGainProc.prepare(spec);
    makeupGainProc.prepare(spec);

    for (auto& f : filters)
        f.prepare(spec);

    distortion.prepare(spec);
    magicEq.prepare(spec);

    compressor.prepare(spec);

    for (auto& lfo : lfos)
    {
        lfo.prepare(sampleRate);
        lfo.reset();
    }
    envFollower.prepare(sampleRate);
    envFollower.reset();

    modValues.fill(0.0f);
}

void PhatForgeAudioProcessor::releaseResources() {}

bool PhatForgeAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void PhatForgeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    const int numSamples = buffer.getNumSamples();

    // --- Pull raw parameter values -----------------------------------
    auto get = [this](const juce::String& id) { return apvts.getRawParameterValue(id)->load(); };

    // --- Advance modulation sources once per block --------------------
    for (int i = 0; i < 2; ++i)
    {
        lfos[(size_t) i].setRate(get(ID::lfoRate[i]));
        lfos[(size_t) i].setWave((int) get(ID::lfoWave[i]));
    }
    envFollower.setAttackMs(get(ID::envAttack));
    envFollower.setReleaseMs(get(ID::envRelease));

    juce::dsp::AudioBlock<float> fullBlock(buffer);
    lastEnvValue = envFollower.processBlock(fullBlock);

    modValues.fill(0.0f);
    for (int i = 0; i < 2; ++i)
    {
        auto targetIdx = (int) get(ID::lfoTarget[i]);
        auto depth     = get(ID::lfoDepth[i]);
        float lfoVal   = lfos[(size_t) i].advanceAndGetValue(numSamples);
        if (targetIdx != 0)
            modValues[(size_t) targetIdx] += lfoVal * depth;
    }
    {
        auto targetIdx = (int) get(ID::envTarget);
        auto envAmt    = get(ID::envAmount);
        if (targetIdx != 0)
            modValues[(size_t) targetIdx] += lastEnvValue * envAmt;
    }

    auto modFor = [this](ModTarget t) { return modValues[(size_t) t]; };

    // --- Gain staging ---------------------------------------------------
    inputGainProc.setGainDecibels(get(ID::inputGain));
    outputGainProc.setGainDecibels(get(ID::outputGain));
    makeupGainProc.setGainDecibels(get(ID::compMakeup));

    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer, true);

    juce::dsp::AudioBlock<float> block(buffer);
    inputGainProc.process(juce::dsp::ProcessContextReplacing<float>(block));

    // --- Filters (with modulation on cutoff frequency) -------------------
    for (int i = 0; i < 3; ++i)
    {
        filters[(size_t) i].setEnabled(get(ID::filterOn[i]) > 0.5f);
        filters[(size_t) i].setType((int) get(ID::filterType[i]));

        float freqMod = modFor((ModTarget) (1 + i)) * 8000.0f; // Filter1Freq..Filter3Freq offsets
        filters[(size_t) i].setFrequency(get(ID::filterFreq[i]) + freqMod);
        filters[(size_t) i].setResonance(get(ID::filterRes[i]));
        filters[(size_t) i].process(block);
    }

    // --- Distortion -------------------------------------------------------
    distortion.setType((Distortion::Type) (int) get(ID::distType));
    float driveMod = modFor(ModTarget::DistDrive);
    distortion.setDrive(get(ID::distDrive) + driveMod);
    distortion.setTone(get(ID::distTone));
    distortion.setMix(get(ID::distMix));
    distortion.process(block);

    // --- Magic EQ -----------------------------------------------------
    magicEq.setFrequency(get(ID::eqFreq));
    float eqMod = modFor(ModTarget::EqAmount);
    magicEq.setAmount(get(ID::eqAmount) + eqMod);
    magicEq.process(block);

    // --- Compressor ------------------------------------------------------
    float threshMod = modFor(ModTarget::CompThreshold) * 24.0f;
    compressor.setThreshold(get(ID::compThreshold) + threshMod);
    compressor.setRatio(get(ID::compRatio));
    compressor.setAttack(get(ID::compAttack));
    compressor.setRelease(get(ID::compRelease));
    compressor.process(juce::dsp::ProcessContextReplacing<float>(block));
    makeupGainProc.process(juce::dsp::ProcessContextReplacing<float>(block));

    outputGainProc.process(juce::dsp::ProcessContextReplacing<float>(block));

    // --- Final dry/wet blend --------------------------------------------
    const float mixAmt = get(ID::mix);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* wet = buffer.getWritePointer(ch);
        auto* dry = dryBuffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
            wet[i] = dry[i] * (1.0f - mixAmt) + wet[i] * mixAmt;
    }

    // --- Metering (peak with a gentle decay, for the GUI) -----------------
    auto updateMeter = [numSamples](std::atomic<float>& meter, const float* data)
    {
        float peak = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            peak = juce::jmax(peak, std::abs(data[i]));

        float current = meter.load();
        float target  = juce::jmax(peak, current * 0.9f); // fast attack, slow-ish decay
        meter.store(target);
    };

    if (buffer.getNumChannels() > 0)
        updateMeter(outputLevelL, buffer.getReadPointer(0));
    if (buffer.getNumChannels() > 1)
        updateMeter(outputLevelR, buffer.getReadPointer(1));
    else if (buffer.getNumChannels() > 0)
        outputLevelR.store(outputLevelL.load());
}

void PhatForgeAudioProcessor::randomiseParameters()
{
    juce::Random rng;
    for (auto* p : getParameters())
    {
        auto* withID = dynamic_cast<juce::AudioProcessorParameterWithID*>(p);
        if (withID == nullptr)
            continue;

        // Keep on/off filter switches and the master mix/gain out of the
        // randomiser so a random click doesn't produce silence or a huge
        // level jump - everything else is fair game, just like the classic
        // "Randomize" button on hardware-style colouring plugins.
        const auto& id = withID->paramID;
        if (id == ID::mix || id == ID::inputGain || id == ID::outputGain)
            continue;

        p->setValueNotifyingHost(rng.nextFloat());
    }
}

void PhatForgeAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    xml->setAttribute("presetName", presetManager->getCurrentPresetName());
    copyXmlToBinary(*xml, destData);
}

void PhatForgeAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
        presetManager->setCurrentPresetNameSilently(xml->getStringAttribute("presetName", "Init"));
    }
}

juce::AudioProcessorEditor* PhatForgeAudioProcessor::createEditor()
{
    return new PhatForgeAudioProcessorEditor(*this);
}

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PhatForgeAudioProcessor();
}
