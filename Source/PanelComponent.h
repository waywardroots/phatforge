#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PhatLookAndFeel.h"

// A hand-painted panel/group box: rounded dark card, a thin accent stripe
// down the left edge, and a small-caps title baked into the top edge of the
// border (rather than relying on JUCE's stock GroupComponent look).
class PanelComponent : public juce::Component
{
public:
    explicit PanelComponent(const juce::String& titleText, juce::Colour accent = PhatLookAndFeel::colours::accent)
        : title(titleText), accentColour(accent)
    {
    }

    void addControl(juce::Component* c)
    {
        children.add(c);
        addAndMakeVisible(c);
        resized();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        juce::DropShadow shadow(juce::Colours::black.withAlpha(0.45f), 8, { 0, 3 });
        juce::Path panelPath;
        panelPath.addRoundedRectangle(bounds, 10.0f);
        shadow.drawForPath(g, panelPath);

        juce::ColourGradient grad(PhatLookAndFeel::colours::panel.brighter(0.03f), bounds.getX(), bounds.getY(),
                                   PhatLookAndFeel::colours::panel.darker(0.15f), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(bounds, 10.0f);

        g.setColour(PhatLookAndFeel::colours::panelEdge);
        g.drawRoundedRectangle(bounds.reduced(0.5f), 10.0f, 1.0f);

        // Accent stripe.
        g.setColour(accentColour);
        g.fillRoundedRectangle(bounds.removeFromLeft(4.0f).reduced(0, 8.0f), 2.0f);

        // Title.
        g.setColour(accentColour);
        g.setFont(juce::Font(12.5f, juce::Font::bold));
        g.drawText(title.toUpperCase(), getLocalBounds().reduced(16, 6).removeFromTop(18),
                   juce::Justification::centredLeft);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        b.removeFromTop(26);
        b = b.reduced(12, 6);

        auto count = (int) children.size();
        if (count == 0)
            return;

        auto w = b.getWidth() / count;
        for (auto* c : children)
            c->setBounds(b.removeFromLeft(w).reduced(4));
    }

private:
    juce::String title;
    juce::Colour accentColour;
    juce::Array<juce::Component*> children;
};
