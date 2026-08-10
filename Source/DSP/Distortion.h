#pragma once
#include <cmath>
#include <vector>
#include <juce_dsp/juce_dsp.h>

// Eight distinct colouring/distortion algorithms, selectable at run time.
// This is original DSP code (not derived from any third-party plugin) that
// aims for a broad palette of character, from soft tube warmth through to
// lo-fi crunch, harmonic excitement, diode grit, wavefolding and rectifier
// fuzz.
class Distortion
{
public:
    enum Type { Tube = 0, Crush, Exciter, Mech, Diode, Fold, Rectify, Shaper, NumTypes };

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        hpFilter.prepare(spec);
        hpFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
        hpFilter.setCutoffFrequency(2500.0f);
        toneFilter.prepare(spec);
        toneFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        crushPhase.resize((size_t) spec.numChannels, 0);
        crushHold.resize((size_t) spec.numChannels, 0.0f);
        reset();
    }

    void reset()
    {
        hpFilter.reset();
        toneFilter.reset();
        std::fill(crushPhase.begin(), crushPhase.end(), 0);
        std::fill(crushHold.begin(), crushHold.end(), 0.0f);
    }

    void setType(Type t)      { type = t; }
    void setDrive(float d)    { drive = juce::jlimit(0.0f, 1.0f, d); }
    void setTone(float t)     { tone = juce::jlimit(0.0f, 1.0f, t); }
    void setMix(float m)      { mix = juce::jlimit(0.0f, 1.0f, m); }

    void process(juce::dsp::AudioBlock<float>& block)
    {
        // Tone control also doubles as the lo-pass "warmth" filter applied
        // after the non-linearity, 800Hz .. 12kHz.
        toneFilter.setCutoffFrequency(juce::jmap(tone, 800.0f, 12000.0f));

        const auto driveGain = 1.0f + drive * 24.0f;

        for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
        {
            auto* data = block.getChannelPointer(ch);

            for (size_t i = 0; i < block.getNumSamples(); ++i)
            {
                const float in = data[i];
                float wet = 0.0f;

                switch (type)
                {
                    case Tube:    wet = std::tanh(in * driveGain) / std::tanh(driveGain); break;
                    case Crush:   wet = processCrush(in, ch); break;
                    case Exciter: wet = processExciter(in, ch, driveGain); break;
                    case Mech:    wet = processMech(in, driveGain); break;
                    case Diode:   wet = processDiode(in, driveGain); break;
                    case Fold:    wet = processFold(in, driveGain); break;
                    case Rectify: wet = processRectify(in, driveGain); break;
                    case Shaper:  wet = processShaper(in, driveGain); break;
                    default: break;
                }

                data[i] = in * (1.0f - mix) + wet * mix;
            }
        }

        toneFilter.process(juce::dsp::ProcessContextReplacing<float>(block));
    }

private:
    float processCrush(float in, size_t ch)
    {
        // Bit-depth reduction (drive controls bit depth, 16 down to ~2 bits)
        const float bits = juce::jmap(drive, 16.0f, 2.0f);
        const float levels = std::pow(2.0f, bits);
        float crushed = std::round(in * levels) / levels;

        // Sample & hold style sample-rate reduction.
        const int holdAmount = 1 + (int) (drive * 30.0f);
        auto& phase = crushPhase[ch];
        auto& held  = crushHold[ch];

        if (phase % holdAmount == 0)
            held = crushed;

        phase++;
        return held;
    }

    float processExciter(float in, size_t ch, float driveGain)
    {
        juce::ignoreUnused(ch);
        // Generate upper harmonics from the high band only, then blend back
        // in with the original so the fundamental stays untouched.
        float hp = hpFilter.processSample((int) ch, in);
        float shaped = hp - (hp * hp * hp) / 3.0f;
        shaped = juce::jlimit(-1.0f, 1.0f, shaped * driveGain * 0.5f);
        return in + shaped * 0.6f;
    }

    float processMech(float in, float driveGain)
    {
        float x = in * driveGain * 0.3f;
        // Hard clip for an aggressive, mechanical grind.
        return juce::jlimit(-1.0f, 1.0f, x);
    }

    float processDiode(float in, float driveGain)
    {
        // Asymmetric diode-style clipper - different thresholds for the
        // positive and negative half of the wave generates strong even
        // harmonics, similar to a germanium/silicon diode clamp.
        float x = in * driveGain * 0.5f;
        const float posThresh = 0.65f;
        const float negThresh = 0.9f;

        if (x > posThresh)
            x = posThresh + (1.0f - std::exp(-(x - posThresh))) * 0.3f;
        else if (x < -negThresh)
            x = -negThresh - (1.0f - std::exp((x + negThresh))) * 0.3f;

        return juce::jlimit(-1.0f, 1.0f, x);
    }

    float processFold(float in, float driveGain)
    {
        // Triangle-style wavefolder: instead of clipping, the signal folds
        // back on itself every time it crosses +/-1, producing complex,
        // metallic harmonic content that increases with drive.
        float x = in * (1.0f + driveGain * 0.6f);

        for (int iter = 0; iter < 8 && (x > 1.0f || x < -1.0f); ++iter)
        {
            if (x > 1.0f)  x = 2.0f - x;
            if (x < -1.0f) x = -2.0f - x;
        }
        return x;
    }

    float processRectify(float in, float driveGain)
    {
        // Blend of full-wave rectification with the original signal - adds
        // an aggressive, buzzy octave-up character (classic fuzz-rectifier
        // trick).
        float x = in * driveGain * 0.4f;
        float rectified = std::abs(x) * (x < 0.0f ? -1.0f : 1.0f) - std::abs(x) * 0.5f;
        float blended = juce::jmap(drive, in, rectified);
        return juce::jlimit(-1.0f, 1.0f, blended * 1.5f);
    }

    float processShaper(float in, float driveGain)
    {
        // Chebyshev-style polynomial waveshaper - "tone" morphs between a
        // low-order (warm) and a high-order (buzzy) harmonic series.
        float x = juce::jlimit(-1.0f, 1.0f, in * driveGain * 0.4f);
        const float x2 = x * x;
        const float x3 = x2 * x;
        const float x5 = x3 * x2;
        float lowOrder  = 1.5f * x - 0.5f * x3;                 // 3rd order (odd, warm)
        float highOrder = (5.0f * x3 - x5 * 4.0f) - x;           // higher order, buzzier
        return juce::jlimit(-1.0f, 1.0f, juce::jmap(tone, lowOrder, highOrder));
    }

    Type type = Tube;
    float drive = 0.3f, tone = 0.5f, mix = 1.0f;
    double sampleRate = 44100.0;

    juce::dsp::StateVariableTPTFilter<float> hpFilter, toneFilter;
    std::vector<int> crushPhase;
    std::vector<float> crushHold;
};
