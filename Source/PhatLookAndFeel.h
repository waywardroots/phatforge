#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// PhatForge's custom look: dark charcoal panels, a warm orange/amber accent
// for anything "hot" (drive, resonance...) and a cool teal accent for
// modulation controls, hand-painted rotary knobs with an arc readout, LED
// style toggles, and pill-shaped combo boxes/buttons. Nothing here uses
// stock JUCE widget graphics - every control is painted from scratch.
class PhatLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PhatLookAndFeel()
    {
        setColour(juce::ResizableWindow::backgroundColourId, colours::bg);
        setColour(juce::Slider::textBoxTextColourId, colours::text);
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Label::textColourId, colours::text);
        setColour(juce::ComboBox::textColourId, colours::text);
        setColour(juce::ComboBox::backgroundColourId, colours::panel);
        setColour(juce::PopupMenu::backgroundColourId, colours::panel);
        setColour(juce::PopupMenu::textColourId, colours::text);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, colours::accent.withAlpha(0.35f));
        setColour(juce::TextButton::buttonColourId, colours::accent);
        setColour(juce::TextButton::textColourOffId, juce::Colours::black);
    }

    struct colours
    {
        static inline juce::Colour bg        { 0xff0f0f13 };
        static inline juce::Colour panel      { 0xff1b1b22 };
        static inline juce::Colour panelEdge  { 0xff2c2c37 };
        static inline juce::Colour text       { 0xffe8e6f0 };
        static inline juce::Colour textDim    { 0xff8d8b9c };
        static inline juce::Colour accent     { 0xffff8c3c };  // warm amber - drive / hot stuff
        static inline juce::Colour accent2    { 0xff36e0c8 };  // cool teal - modulation
        static inline juce::Colour trackDark  { 0xff2a2a33 };
    };

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height).reduced(4.0f);
        auto centre = bounds.getCentre();
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        auto angle  = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        const bool isModColour = slider.getProperties().contains("modColour");
        const juce::Colour accentColour = isModColour ? colours::accent2 : colours::accent;

        // Outer body with a subtle vertical gradient + soft drop shadow.
        auto knobBounds = bounds.reduced(radius * 0.22f);
        juce::DropShadow shadow(juce::Colours::black.withAlpha(0.6f), 6, { 0, 2 });
        juce::Path knobPath;
        knobPath.addEllipse(knobBounds);
        shadow.drawForPath(g, knobPath);

        juce::ColourGradient bodyGrad(colours::panelEdge, centre.x, knobBounds.getY(),
                                       colours::panel, centre.x, knobBounds.getBottom(), false);
        g.setGradientFill(bodyGrad);
        g.fillEllipse(knobBounds);
        g.setColour(colours::panelEdge);
        g.drawEllipse(knobBounds, 1.2f);

        // Background arc track.
        const float arcThickness = juce::jmax(2.5f, radius * 0.14f);
        juce::Path track;
        track.addCentredArc(centre.x, centre.y, radius - arcThickness * 0.4f, radius - arcThickness * 0.4f,
                             0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(colours::trackDark);
        g.strokePath(track, juce::PathStrokeType(arcThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Value arc, glowing accent colour.
        juce::Path valueArc;
        valueArc.addCentredArc(centre.x, centre.y, radius - arcThickness * 0.4f, radius - arcThickness * 0.4f,
                                0.0f, rotaryStartAngle, angle, true);
        g.setColour(accentColour.withAlpha(0.35f));
        g.strokePath(valueArc, juce::PathStrokeType(arcThickness * 1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(accentColour);
        g.strokePath(valueArc, juce::PathStrokeType(arcThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Pointer line + centre cap.
        juce::Path pointer;
        auto pointerLength = knobBounds.getHeight() * 0.32f;
        pointer.addRoundedRectangle(-1.6f, -(knobBounds.getHeight() * 0.5f - 3.0f), 3.2f, pointerLength, 1.6f);
        g.setColour(colours::text);
        g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(centre));

        g.setColour(accentColour);
        g.fillEllipse(juce::Rectangle<float>(6.0f, 6.0f).withCentre(centre));
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        juce::ignoreUnused(shouldDrawButtonAsDown);
        auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
        auto ledSize = juce::jmin(16.0f, bounds.getHeight());
        auto ledBounds = juce::Rectangle<float>(ledSize, ledSize).withCentre({ bounds.getX() + ledSize * 0.5f + 2.0f, bounds.getCentreY() });

        const bool on = button.getToggleState();
        auto ledColour = on ? colours::accent : colours::trackDark;

        if (on)
        {
            g.setColour(colours::accent.withAlpha(0.35f));
            g.fillEllipse(ledBounds.expanded(4.0f));
        }

        g.setColour(ledColour);
        g.fillEllipse(ledBounds);
        g.setColour(colours::panelEdge);
        g.drawEllipse(ledBounds, 1.0f);

        g.setColour(shouldDrawButtonAsHighlighted ? colours::text : colours::textDim);
        g.setFont(juce::Font(13.0f));
        auto textBounds = bounds.withTrimmedLeft(ledSize + 10.0f);
        g.drawText(button.getButtonText(), textBounds, juce::Justification::centredLeft);
    }

    void drawComboBox(juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<float>(0, 0, (float) width, (float) height).reduced(1.5f);
        g.setColour(colours::panel);
        g.fillRoundedRectangle(bounds, bounds.getHeight() * 0.5f);
        g.setColour(box.hasKeyboardFocus(true) ? colours::accent : colours::panelEdge);
        g.drawRoundedRectangle(bounds, bounds.getHeight() * 0.5f, 1.2f);

        // Little arrow.
        auto arrowZone = bounds.removeFromRight(bounds.getHeight());
        juce::Path arrow;
        auto c = arrowZone.getCentre();
        arrow.addTriangle(c.x - 4.0f, c.y - 2.5f, c.x + 4.0f, c.y - 2.5f, c.x, c.y + 3.5f);
        g.setColour(colours::textDim);
        g.fillPath(arrow);
    }

    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds(12, 1, box.getWidth() - box.getHeight() - 12, box.getHeight() - 2);
        label.setFont(juce::Font(13.0f));
        label.setColour(juce::Label::textColourId, colours::text);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour&,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.5f);
        auto baseColour = colours::accent;
        if (shouldDrawButtonAsDown)       baseColour = baseColour.darker(0.3f);
        else if (shouldDrawButtonAsHighlighted) baseColour = baseColour.brighter(0.15f);

        juce::ColourGradient grad(baseColour.brighter(0.2f), bounds.getX(), bounds.getY(),
                                   baseColour.darker(0.15f), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(bounds, bounds.getHeight() * 0.5f);
        g.setColour(juce::Colours::black.withAlpha(0.25f));
        g.drawRoundedRectangle(bounds, bounds.getHeight() * 0.5f, 1.0f);
    }

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override
    {
        return juce::Font(juce::jmin(15.0f, (float) buttonHeight * 0.5f), juce::Font::bold);
    }

    juce::Label* createSliderTextBox(juce::Slider& slider) override
    {
        auto* l = LookAndFeel_V4::createSliderTextBox(slider);
        l->setFont(juce::Font(12.0f));
        l->setColour(juce::Label::textColourId, colours::textDim);
        l->setJustificationType(juce::Justification::centred);
        return l;
    }
};
