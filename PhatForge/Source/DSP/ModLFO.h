#pragma once
#include <cmath>
#include <vector>
#include <juce_dsp/juce_dsp.h>

// Simple free-running LFO with four waveshapes, used to modulate any of the
// mod-matrix targets. Runs at block rate (recalculated once per processBlock)
// which is plenty for the slow, musical movement this kind of effect needs.
class ModLFO
{
public:
    enum Wave { Sine = 0, Triangle, Square, Saw };

    void prepare(double newSampleRate) { sampleRate = newSampleRate; }
    void reset() { phase = 0.0; }

    void setRate(float hz)  { rate = hz; }
    void setWave(int w)     { wave = (Wave) w; }

    // Advances the LFO by numSamples and returns the current value in [-1, 1]
    float advanceAndGetValue(int numSamples)
    {
        const double value = render(phase);
        phase += rate * numSamples / sampleRate;
        phase -= std::floor(phase);
        return (float) value;
    }

private:
    double render(double p) const
    {
        switch (wave)
        {
            default:
            case Sine:     return std::sin(p * juce::MathConstants<double>::twoPi);
            case Triangle: return 1.0 - 4.0 * std::abs(std::round(p - 0.25) - (p - 0.25));
            case Square:   return p < 0.5 ? 1.0 : -1.0;
            case Saw:      return 2.0 * (p - std::floor(p + 0.5));
        }
    }

    double sampleRate = 44100.0;
    double phase = 0.0;
    float rate = 1.0f;
    Wave wave = Sine;
};
