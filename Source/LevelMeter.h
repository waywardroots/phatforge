#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PhatLookAndFeel.h"
#include "PluginProcessor.h"

// Small custom-painted stereo peak meter with a falling peak-hold line,
// polling the processor's lock-free level atomics at 30Hz.
class LevelMeter : public juce::Component, private juce::Timer
{
public:
    explicit LevelMeter(PhatForgeAudioProcessor& p) : processor(p)
    {
        startTimerHz(30);
    }

    ~LevelMeter() override { stopTimer(); }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(PhatLookAndFeel::colours::panel.darker(0.2f));
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(PhatLookAndFeel::colours::panelEdge);
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);

        auto inner = bounds.reduced(3.0f);
        auto barWidth = (inner.getWidth() - 4.0f) * 0.5f;

        drawChannel(g, inner.removeFromLeft(barWidth), levelSmoothedL, peakL);
        inner.removeFromLeft(4.0f);
        drawChannel(g, inner, levelSmoothedR, peakR);
    }

    void resized() override {}

private:
    void drawChannel(juce::Graphics& g, juce::Rectangle<float> area, float level, float peak)
    {
        const float h = area.getHeight();
        auto dbToY = [&](float db)
        {
            auto norm = juce::jmap(juce::jlimit(-48.0f, 6.0f, db), -48.0f, 6.0f, 0.0f, 1.0f);
            return area.getBottom() - norm * h;
        };

        auto levelDb = juce::Decibels::gainToDecibels(juce::jmax(level, 0.0001f));
        auto peakDb  = juce::Decibels::gainToDecibels(juce::jmax(peak, 0.0001f));

        auto filled = area.withTop(dbToY(levelDb));
        juce::ColourGradient grad(juce::Colours::limegreen, area.getX(), area.getBottom(),
                                   juce::Colours::red, area.getX(), area.getY(), false);
        grad.addColour(0.7, juce::Colours::orange);
        g.setGradientFill(grad);
        g.fillRect(filled);

        // Peak-hold line.
        auto peakY = dbToY(peakDb);
        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.fillRect(area.getX(), peakY - 1.0f, area.getWidth(), 2.0f);
    }

    void timerCallback() override
    {
        auto newL = processor.getOutputLevel(0);
        auto newR = processor.getOutputLevel(1);

        levelSmoothedL = juce::jmax(newL, levelSmoothedL * 0.7f);
        levelSmoothedR = juce::jmax(newR, levelSmoothedR * 0.7f);

        peakL = juce::jmax(newL, peakL * 0.97f);
        peakR = juce::jmax(newR, peakR * 0.97f);

        repaint();
    }

    PhatForgeAudioProcessor& processor;
    float levelSmoothedL = 0.0f, levelSmoothedR = 0.0f;
    float peakL = 0.0f, peakR = 0.0f;
};
