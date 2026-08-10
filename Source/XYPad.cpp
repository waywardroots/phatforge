#include "XYPad.h"
#include "PhatLookAndFeel.h"

XYPad::XYPad(juce::RangedAudioParameter& xParamIn, juce::RangedAudioParameter& yParamIn)
    : xParam(xParamIn), yParam(yParamIn)
{
    startTimerHz(30);
}

XYPad::~XYPad() { stopTimer(); }

void XYPad::paint(juce::Graphics& g)
{
    using C = PhatLookAndFeel::colours;
    auto bounds = getLocalBounds().toFloat();

    juce::DropShadow shadow(juce::Colours::black.withAlpha(0.5f), 8, { 0, 3 });
    juce::Path panelPath;
    panelPath.addRoundedRectangle(bounds, 10.0f);
    shadow.drawForPath(g, panelPath);

    juce::ColourGradient bg(C::panel.brighter(0.05f), bounds.getX(), bounds.getY(),
                             C::bg, bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(bg);
    g.fillRoundedRectangle(bounds, 10.0f);
    g.setColour(C::panelEdge);
    g.drawRoundedRectangle(bounds.reduced(1.0f), 10.0f, 1.2f);

    auto grid = bounds.reduced(8.0f);
    g.setColour(C::panelEdge.withAlpha(0.6f));
    for (float f = 0.25f; f < 1.0f; f += 0.25f)
    {
        g.drawHorizontalLine((int) (grid.getY() + grid.getHeight() * f), grid.getX(), grid.getRight());
        g.drawVerticalLine((int) (grid.getX() + grid.getWidth() * f), grid.getY(), grid.getBottom());
    }

    auto dotX = grid.getX() + dotPosNormalised.x * grid.getWidth();
    auto dotY = grid.getY() + (1.0f - dotPosNormalised.y) * grid.getHeight();

    // Crosshair guides.
    g.setColour(C::accent2.withAlpha(0.35f));
    g.drawVerticalLine((int) dotX, grid.getY(), grid.getBottom());
    g.drawHorizontalLine((int) dotY, grid.getX(), grid.getRight());

    // Glowing dot.
    g.setColour(C::accent.withAlpha(0.25f));
    g.fillEllipse(juce::Rectangle<float>(26.0f, 26.0f).withCentre({ dotX, dotY }));
    g.setColour(C::accent);
    g.fillEllipse(juce::Rectangle<float>(12.0f, 12.0f).withCentre({ dotX, dotY }));
    g.setColour(juce::Colours::white);
    g.drawEllipse(juce::Rectangle<float>(12.0f, 12.0f).withCentre({ dotX, dotY }), 1.5f);
}

void XYPad::resized() {}

void XYPad::mouseDown(const juce::MouseEvent& e) { setFromMousePosition(e.position); }
void XYPad::mouseDrag(const juce::MouseEvent& e) { setFromMousePosition(e.position); }

void XYPad::setFromMousePosition(juce::Point<float> p)
{
    auto grid = getLocalBounds().toFloat().reduced(8.0f);
    float nx = juce::jlimit(0.0f, 1.0f, (p.x - grid.getX()) / grid.getWidth());
    float ny = juce::jlimit(0.0f, 1.0f, 1.0f - ((p.y - grid.getY()) / grid.getHeight()));

    dotPosNormalised = { nx, ny };

    xParam.setValueNotifyingHost(nx);
    yParam.setValueNotifyingHost(ny);
    repaint();
}

void XYPad::timerCallback()
{
    // Keep the dot in sync if the parameters change from automation/presets.
    auto nx = xParam.getValue();
    auto ny = yParam.getValue();
    if (std::abs(nx - dotPosNormalised.x) > 0.001f || std::abs(ny - dotPosNormalised.y) > 0.001f)
    {
        dotPosNormalised = { nx, ny };
        repaint();
    }
}
