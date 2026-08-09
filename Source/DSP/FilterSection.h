#pragma once
#include <cmath>
#include <vector>
#include <juce_dsp/juce_dsp.h>

// A single switchable-type resonant filter. The plugin instantiates three of
// these in series. Covers eight distinct characters:
//   0 Low Pass    1 High Pass   2 Band Pass   3 Notch
//   4 Peak/Bell   5 Low Shelf   6 High Shelf  7 Comb (resonant delay-line)
//
// Types 0-2 use JUCE's State Variable (TPT) filter for a smooth, cheap,
// self-resonant multimode response. Types 3-6 use classic RBJ biquad
// coefficients via juce::dsp::IIR. Type 7 is a small hand-rolled resonant
// comb filter for a metallic/robotic flavour that the others can't do.
class PhatFilter
{
public:
    enum Type { LowPass = 0, HighPass, BandPass, Notch, Peak, LowShelf, HighShelf, Comb, NumTypes };

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        svFilter.prepare(spec);
        iirFilter.prepare(spec);

        combBuffer.assign((size_t) spec.numChannels, std::vector<float>((size_t) sampleRate / 20 + 4, 0.0f));
        combWritePos.assign((size_t) spec.numChannels, 0);

        reset();
    }

    void reset()
    {
        svFilter.reset();
        iirFilter.reset();
        for (auto& buf : combBuffer)
            std::fill(buf.begin(), buf.end(), 0.0f);
        std::fill(combWritePos.begin(), combWritePos.end(), 0);
    }

    void setType(int typeIndex) { type = (Type) juce::jlimit(0, (int) NumTypes - 1, typeIndex); }
    void setFrequency(float f)  { freq = juce::jlimit(20.0f, 20000.0f, f); }
    void setResonance(float r)  { resonance = juce::jlimit(0.1f, 10.0f, r); }
    void setEnabled(bool e)     { enabled = e; }

    void process(juce::dsp::AudioBlock<float>& block)
    {
        if (! enabled)
            return;

        switch (type)
        {
            case LowPass:
            case HighPass:
            case BandPass:
                processStateVariable(block);
                break;

            case Notch:
            case Peak:
            case LowShelf:
            case HighShelf:
                processIIR(block);
                break;

            case Comb:
                processComb(block);
                break;

            default: break;
        }
    }

private:
    void processStateVariable(juce::dsp::AudioBlock<float>& block)
    {
        using T = juce::dsp::StateVariableTPTFilterType;
        svFilter.setType(type == LowPass ? T::lowpass : (type == HighPass ? T::highpass : T::bandpass));
        svFilter.setCutoffFrequency(freq);
        svFilter.setResonance(resonance);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        svFilter.process(ctx);
    }

    void processIIR(juce::dsp::AudioBlock<float>& block)
    {
        const float q = juce::jlimit(0.2f, 10.0f, resonance);
        const float gainDb = juce::jmap(resonance, 0.1f, 10.0f, -18.0f, 18.0f);

        switch (type)
        {
            case Notch:     *iirFilter.state = *juce::dsp::IIR::Coefficients<float>::makeNotch(sampleRate, freq, q); break;
            case Peak:      *iirFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, freq, q, juce::Decibels::decibelsToGain(gainDb)); break;
            case LowShelf:  *iirFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, freq, 0.7f, juce::Decibels::decibelsToGain(gainDb)); break;
            case HighShelf: *iirFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, freq, 0.7f, juce::Decibels::decibelsToGain(gainDb)); break;
            default: break;
        }

        juce::dsp::ProcessContextReplacing<float> ctx(block);
        iirFilter.process(ctx);
    }

    void processComb(juce::dsp::AudioBlock<float>& block)
    {
        // Simple feedback comb filter: delay time set by frequency, feedback
        // amount set by resonance. Creates a buzzy, resonant/robotic tone
        // colour rather than a smooth spectral shape.
        const float delaySeconds = 1.0f / juce::jmax(20.0f, freq);
        const int delaySamples = juce::jlimit(2, (int) combBuffer[0].size() - 2,
                                               (int) std::round(delaySeconds * (float) sampleRate));
        const float feedback = juce::jmap(resonance, 0.1f, 10.0f, 0.15f, 0.97f);

        for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
        {
            auto* data = block.getChannelPointer(ch);
            auto& buf = combBuffer[ch];
            auto& writePos = combWritePos[ch];
            const int bufSize = (int) buf.size();

            for (size_t i = 0; i < block.getNumSamples(); ++i)
            {
                const int readPos = (writePos - delaySamples + bufSize) % bufSize;
                const float delayed = buf[(size_t) readPos];
                const float x = data[i] + delayed * feedback;
                buf[(size_t) writePos] = x;
                data[i] = 0.5f * data[i] + 0.5f * delayed;
                writePos = (writePos + 1) % bufSize;
            }
        }
    }

    bool enabled = false;
    Type type = LowPass;
    float freq = 1000.0f, resonance = 0.7f;
    double sampleRate = 44100.0;

    juce::dsp::StateVariableTPTFilter<float> svFilter;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> iirFilter;

    std::vector<std::vector<float>> combBuffer;
    std::vector<int> combWritePos;
};
