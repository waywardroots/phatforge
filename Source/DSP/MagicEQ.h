#pragma once
#include <cmath>
#include <vector>
#include <juce_dsp/juce_dsp.h>

// "Magic EQ" - a low-end enhancer. It splits off a band around a settable
// frequency, runs it through a gentle non-linearity to generate reinforcing
// harmonics (so it still reads on small speakers), and blends that back in
// underneath a low shelf boost. Original implementation.
class MagicEQ
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        lowSplit.prepare(spec);
        lowSplit.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        shelf.prepare(spec);
        updateShelf();
    }

    void reset()
    {
        lowSplit.reset();
        shelf.reset();
    }

    void setFrequency(float f) { freq = f; }
    void setAmount(float a)    { amount = juce::jlimit(0.0f, 1.0f, a); }

    void process(juce::dsp::AudioBlock<float>& block)
    {
        updateShelf();
        lowSplit.setCutoffFrequency(freq);

        for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
        {
            auto* data = block.getChannelPointer(ch);
            for (size_t i = 0; i < block.getNumSamples(); ++i)
            {
                float low = lowSplit.processSample((int) ch, data[i]);
                float sat = std::tanh(low * (1.0f + amount * 6.0f));
                data[i] += sat * amount * 0.5f;
            }
        }

        juce::dsp::ProcessContextReplacing<float> ctx(block);
        shelf.process(ctx);
    }

private:
    void updateShelf()
    {
        *shelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, freq, 0.7f,
                                                                            juce::Decibels::decibelsToGain(amount * 9.0f));
    }

    double sampleRate = 44100.0;
    float freq = 120.0f, amount = 0.0f;

    juce::dsp::StateVariableTPTFilter<float> lowSplit;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> shelf;
};
