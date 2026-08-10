#pragma once
#include <cmath>
#include <vector>
#include <juce_dsp/juce_dsp.h>

// One-pole envelope follower used as another modulation source, exactly like
// the envelope follower found on classic multi-effect "phattening" units.
class EnvelopeFollower
{
public:
    void prepare(double newSampleRate) { sampleRate = newSampleRate; }
    void reset() { envelope = 0.0f; }

    void setAttackMs(float ms)  { attackMs = ms; }
    void setReleaseMs(float ms) { releaseMs = ms; }

    // Feed it a block, get back the peak envelope value in [0, 1] for that block.
    float processBlock(const juce::dsp::AudioBlock<float>& block)
    {
        const float attackCoeff  = (float) std::exp(-1.0 / (0.001 * attackMs  * sampleRate));
        const float releaseCoeff = (float) std::exp(-1.0 / (0.001 * releaseMs * sampleRate));

        float peak = 0.0f;
        for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
        {
            auto* data = block.getChannelPointer(ch);
            for (size_t i = 0; i < block.getNumSamples(); ++i)
            {
                const float rectified = std::abs(data[i]);
                const float coeff = rectified > envelope ? attackCoeff : releaseCoeff;
                envelope = coeff * (envelope - rectified) + rectified;
            }
            peak = juce::jmax(peak, envelope);
        }
        return juce::jlimit(0.0f, 1.0f, peak);
    }

private:
    double sampleRate = 44100.0;
    float attackMs = 5.0f, releaseMs = 150.0f;
    float envelope = 0.0f;
};
