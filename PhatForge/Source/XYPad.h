#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

// A simple X/Y touch pad that drives two arbitrary APVTS parameters at once -
// the same idea as the X/Y pad found on classic colouring multi-effects,
// used here for quick, expressive real-time tweaks.
class XYPad : public juce::Component, private juce::Timer
{
public:
    XYPad(juce::RangedAudioParameter& xParam, juce::RangedAudioParameter& yParam);
    ~XYPad() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

private:
    void timerCallback() override;
    void setFromMousePosition(juce::Point<float> p);

    juce::RangedAudioParameter& xParam;
    juce::RangedAudioParameter& yParam;
    juce::Point<float> dotPosNormalised { 0.5f, 0.5f };
};
