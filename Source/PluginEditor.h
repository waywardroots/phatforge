#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "XYPad.h"
#include "PanelComponent.h"
#include "LevelMeter.h"
#include "PhatLookAndFeel.h"
#include "PresetBar.h"

// Small helper that bundles a rotary slider with its own label and APVTS
// attachment so the editor constructor doesn't turn into an unreadable wall
// of boilerplate.
struct KnobControl : public juce::Component
{
    KnobControl(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                const juce::String& labelText, bool useModColour = false)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 62, 15);
        slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        if (useModColour)
            slider.getProperties().set("modColour", true);
        addAndMakeVisible(slider);

        label.setText(labelText, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(11.5f));
        label.setColour(juce::Label::textColourId, PhatLookAndFeel::colours::textDim);
        addAndMakeVisible(label);

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, slider);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        label.setBounds(b.removeFromBottom(15));
        slider.setBounds(b);
    }

    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

struct ComboControl : public juce::Component
{
    ComboControl(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& labelText)
    {
        box.addItemList(dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(paramId))->choices, 1);
        addAndMakeVisible(box);

        label.setText(labelText, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(11.5f));
        label.setColour(juce::Label::textColourId, PhatLookAndFeel::colours::textDim);
        addAndMakeVisible(label);

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, paramId, box);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        label.setBounds(b.removeFromBottom(15));
        box.setBounds(b.reduced(0, 10));
    }

    juce::ComboBox box;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
};

struct ToggleControl : public juce::Component
{
    ToggleControl(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& labelText)
    {
        button.setButtonText(labelText);
        addAndMakeVisible(button);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, paramId, button);
    }

    void resized() override { button.setBounds(getLocalBounds()); }

    juce::ToggleButton button;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
};

class PhatForgeAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit PhatForgeAudioProcessorEditor(PhatForgeAudioProcessor&);
    ~PhatForgeAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void paintHeader(juce::Graphics& g, juce::Rectangle<int> area);

    PhatForgeAudioProcessor& processor;
    PhatLookAndFeel lookAndFeel;

    juce::Viewport viewport;
    juce::Component content;

    std::vector<std::unique_ptr<juce::Component>> owned;
    std::vector<std::unique_ptr<PanelComponent>> panels;

    XYPad xyPad;
    juce::Label xyLabel;
    juce::TextButton randomiseButton { "RANDOMIZE" };
    LevelMeter meter;
    PresetBar presetBar;

    juce::Rectangle<int> headerArea;

    PanelComponent& addPanel(const juce::String& title, int height, juce::Colour accent);
    KnobControl& addKnob(PanelComponent& s, const juce::String& paramId, const juce::String& text, bool modColour = false);
    ComboControl& addCombo(PanelComponent& s, const juce::String& paramId, const juce::String& text);
    ToggleControl& addToggle(PanelComponent& s, const juce::String& paramId, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhatForgeAudioProcessorEditor)
};
