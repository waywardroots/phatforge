#pragma once
#include <cmath>
#include <vector>
#include <juce_dsp/juce_dsp.h>

// "Magic EQ" - a low-end enhancer. It splits off a band around a settable
// frequency, runs it through a gentle non-linearity to generate reinforcing
// harmonics (so it still reads on small speakers), and blends that back in
// underneath a low shelf boost. Original implementation.
//
// Gain staging: the saturated low-band blend and the low-shelf boost both
// add energy, so their combined effect is automatically compensated (see
// updateShelf()/compensation below) and safety-limited (see softClip())
// so raising Amount reinforces the low end tonally without the module
// itself becoming a source of clipping - regardless of how hot the input
// is or how far Amount is pushed.
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

        // Bypass cleanly when the effect is dialled out, so Amount == 0 is
        // a true, bit-transparent pass-through (aside from tracking the
        // low-split filter's own state, which stays inert with no signal
        // fed into the blend).
        if (amount <= 0.0f)
            return;

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

        // Automatic makeup-gain compensation: the shelf boost and the
        // saturated blend above both add level, so as the boost grows we
        // trim the overall output back down (partial/sqrt compensation -
        // enough to stop peak level from ballooning with Amount, while
        // still leaving the intended extra low-end weight audible).
        const float compensation = 1.0f / std::sqrt(shelfGainLinear);

        for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
        {
            auto* data = block.getChannelPointer(ch);
            for (size_t i = 0; i < block.getNumSamples(); ++i)
                data[i] = softClip(data[i] * compensation);
        }
    }

private:
    // Transparent (identity) up to +/-0.9, then gently rounds off instead of
    // hard-clipping beyond that. This is the final safety net that keeps any
    // Amount/input-level combination from launching hot into the next stage
    // of the chain (compressor, filters, output gain).
    static float softClip(float x)
    {
        constexpr float threshold = 0.9f;
        const float ax = std::abs(x);
        if (ax <= threshold)
            return x;

        const float sign = x < 0.0f ? -1.0f : 1.0f;
        const float over = ax - threshold;
        return sign * (threshold + (1.0f - threshold) * std::tanh(over / (1.0f - threshold)));
    }

    void updateShelf()
    {
        const float gainDb = amount * 9.0f;
        shelfGainLinear = juce::Decibels::decibelsToGain(gainDb);
        *shelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, freq, 0.7f, shelfGainLinear);
    }

    double sampleRate = 44100.0;
    float freq = 120.0f, amount = 0.0f;
    float shelfGainLinear = 1.0f;

    juce::dsp::StateVariableTPTFilter<float> lowSplit;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> shelf;
};
