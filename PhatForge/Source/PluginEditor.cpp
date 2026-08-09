#include "PluginEditor.h"

PanelComponent& PhatForgeAudioProcessorEditor::addPanel(const juce::String& title, int height, juce::Colour accent)
{
    auto p = std::make_unique<PanelComponent>(title, accent);
    content.addAndMakeVisible(*p);
    p->setSize(760, height);
    panels.push_back(std::move(p));
    return *panels.back();
}

KnobControl& PhatForgeAudioProcessorEditor::addKnob(PanelComponent& s, const juce::String& paramId, const juce::String& text, bool modColour)
{
    auto c = std::make_unique<KnobControl>(processor.apvts, paramId, text, modColour);
    auto& ref = *c;
    s.addControl(c.get());
    owned.push_back(std::move(c));
    return ref;
}

ComboControl& PhatForgeAudioProcessorEditor::addCombo(PanelComponent& s, const juce::String& paramId, const juce::String& text)
{
    auto c = std::make_unique<ComboControl>(processor.apvts, paramId, text);
    auto& ref = *c;
    s.addControl(c.get());
    owned.push_back(std::move(c));
    return ref;
}

ToggleControl& PhatForgeAudioProcessorEditor::addToggle(PanelComponent& s, const juce::String& paramId, const juce::String& text)
{
    auto c = std::make_unique<ToggleControl>(processor.apvts, paramId, text);
    auto& ref = *c;
    s.addControl(c.get());
    owned.push_back(std::move(c));
    return ref;
}

PhatForgeAudioProcessorEditor::PhatForgeAudioProcessorEditor(PhatForgeAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p),
      xyPad(*processor.apvts.getParameter(ID::distDrive), *processor.apvts.getParameter(ID::filterFreq[0])),
      meter(p),
      presetBar(p.getPresetManager())
{
    setLookAndFeel(&lookAndFeel);
    using C = PhatLookAndFeel::colours;

    addAndMakeVisible(presetBar);

    addAndMakeVisible(xyPad);
    xyLabel.setText("X/Y PAD  \xc2\xb7  X = Drive   Y = Filter 1 Freq", juce::dontSendNotification);
    xyLabel.setJustificationType(juce::Justification::centred);
    xyLabel.setFont(juce::Font(11.0f));
    xyLabel.setColour(juce::Label::textColourId, C::textDim);
    addAndMakeVisible(xyLabel);

    addAndMakeVisible(randomiseButton);
    randomiseButton.onClick = [this] { processor.randomiseParameters(); };

    addAndMakeVisible(meter);

    // --- Global -------------------------------------------------------
    auto& global = addPanel("Input / Output", 100, C::accent);
    addKnob(global, ID::inputGain, "Input");
    addKnob(global, ID::outputGain, "Output");
    addKnob(global, ID::mix, "Dry/Wet");

    // --- Filters --------------------------------------------------------
    for (int i = 0; i < 3; ++i)
    {
        auto& filt = addPanel("Filter " + juce::String(i + 1), 100, C::accent);
        addToggle(filt, ID::filterOn[i], "On");
        addCombo(filt, ID::filterType[i], "Type");
        addKnob(filt, ID::filterFreq[i], "Freq");
        addKnob(filt, ID::filterRes[i], "Res/Gain");
    }

    // --- Distortion -------------------------------------------------------
    auto& dist = addPanel("Distortion", 100, C::accent);
    addCombo(dist, ID::distType, "Type");
    addKnob(dist, ID::distDrive, "Drive");
    addKnob(dist, ID::distTone, "Tone");
    addKnob(dist, ID::distMix, "Mix");

    // --- Magic EQ ---------------------------------------------------------
    auto& eq = addPanel("Magic EQ", 100, C::accent);
    addKnob(eq, ID::eqFreq, "Freq");
    addKnob(eq, ID::eqAmount, "Amount");

    // --- Compressor ------------------------------------------------------
    auto& comp = addPanel("Compressor", 100, C::accent);
    addKnob(comp, ID::compThreshold, "Thresh");
    addKnob(comp, ID::compRatio, "Ratio");
    addKnob(comp, ID::compAttack, "Attack");
    addKnob(comp, ID::compRelease, "Release");
    addKnob(comp, ID::compMakeup, "Makeup");

    // --- LFOs (teal / mod colour) -----------------------------------------
    for (int i = 0; i < 2; ++i)
    {
        auto& lfo = addPanel("LFO " + juce::String(i + 1), 100, C::accent2);
        addKnob(lfo, ID::lfoRate[i], "Rate", true);
        addKnob(lfo, ID::lfoDepth[i], "Depth", true);
        addCombo(lfo, ID::lfoWave[i], "Wave");
        addCombo(lfo, ID::lfoTarget[i], "Target");
    }

    // --- Envelope follower (teal / mod colour) -----------------------------
    auto& env = addPanel("Envelope Follower", 100, C::accent2);
    addKnob(env, ID::envAmount, "Amount", true);
    addKnob(env, ID::envAttack, "Attack", true);
    addKnob(env, ID::envRelease, "Release", true);
    addCombo(env, ID::envTarget, "Target");

    // Stack all panels vertically inside the scrollable content component.
    int y = 8;
    for (auto& s : panels)
    {
        s->setBounds(0, y, 760, s->getHeight());
        y += s->getHeight() + 8;
    }
    content.setSize(760, y + 8);

    viewport.setViewedComponent(&content, false);
    viewport.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport);

    setResizable(true, true);
    setResizeLimits(800, 520, 1100, 1200);
    setSize(800, 800);
}

PhatForgeAudioProcessorEditor::~PhatForgeAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void PhatForgeAudioProcessorEditor::paint(juce::Graphics& g)
{
    using C = PhatLookAndFeel::colours;
    auto bounds = getLocalBounds();

    juce::ColourGradient bg(C::bg.brighter(0.02f), 0.0f, 0.0f, C::bg.darker(0.35f), 0.0f, (float) bounds.getHeight(), false);
    g.setGradientFill(bg);
    g.fillAll();

    // Faint diagonal accent stripes in the header band for a bit of texture.
    paintHeader(g, headerArea);
}

void PhatForgeAudioProcessorEditor::paintHeader(juce::Graphics& g, juce::Rectangle<int> area)
{
    using C = PhatLookAndFeel::colours;
    if (area.isEmpty())
        return;

    g.saveState();
    g.reduceClipRegion(area);

    juce::ColourGradient panelGrad(C::panel, (float) area.getX(), (float) area.getY(),
                                    C::panel.darker(0.25f), (float) area.getX(), (float) area.getBottom(), false);
    g.setGradientFill(panelGrad);
    g.fillRect(area);

    g.setColour(juce::Colours::white.withAlpha(0.02f));
    for (int x = area.getX() - area.getHeight(); x < area.getRight(); x += 26)
    {
        juce::Path p;
        p.startNewSubPath((float) x, (float) area.getBottom());
        p.lineTo((float) (x + area.getHeight()), (float) area.getY());
        g.strokePath(p, juce::PathStrokeType(10.0f));
    }

    g.setColour(C::panelEdge);
    g.drawLine((float) area.getX(), (float) area.getBottom(), (float) area.getRight(), (float) area.getBottom(), 1.5f);
    g.restoreState();

    // Logo mark: a rounded square with a stylised "P" wave-bar glyph.
    auto logoBounds = juce::Rectangle<float>(18.0f, area.getCentreY() - 20.0f, 40.0f, 40.0f);
    juce::ColourGradient logoGrad(C::accent, logoBounds.getX(), logoBounds.getY(),
                                   C::accent2, logoBounds.getRight(), logoBounds.getBottom(), false);
    g.setGradientFill(logoGrad);
    g.fillRoundedRectangle(logoBounds, 10.0f);

    g.setColour(juce::Colours::black.withAlpha(0.85f));
    juce::Path bars;
    bars.addRoundedRectangle(logoBounds.getX() + 9.0f,  logoBounds.getBottom() - 14.0f, 5.0f, 10.0f, 1.5f);
    bars.addRoundedRectangle(logoBounds.getX() + 17.5f, logoBounds.getBottom() - 22.0f, 5.0f, 18.0f, 1.5f);
    bars.addRoundedRectangle(logoBounds.getX() + 26.0f, logoBounds.getBottom() - 30.0f, 5.0f, 26.0f, 1.5f);
    g.fillPath(bars);

    // Title text with soft drop shadow for a bit of depth.
    auto textArea = area.withTrimmedLeft(72).withTrimmedRight(200);
    auto titleY = area.getCentreY() - 24;
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.setFont(juce::Font(26.0f, juce::Font::bold));
    g.drawText("PhatForge", textArea.withY(titleY + 1).withHeight(32), juce::Justification::topLeft);
    g.setColour(C::text);
    g.drawText("PhatForge", textArea.withY(titleY).withHeight(32), juce::Justification::topLeft);

    g.setColour(C::textDim);
    g.setFont(juce::Font(12.0f));
    g.drawText("phattening multi-effect  -  distortion / filters / dynamics / modulation",
               textArea.withY(titleY + 32).withHeight(20), juce::Justification::topLeft);
}

void PhatForgeAudioProcessorEditor::resized()
{
    auto b = getLocalBounds();

    headerArea = b.removeFromTop(150);

    auto controlsRow = headerArea;
    controlsRow.removeFromLeft(juce::jmax(0, getWidth() - 380));
    controlsRow = controlsRow.reduced(12, 14);

    auto xyArea = controlsRow.removeFromLeft(190);
    xyLabel.setBounds(xyArea.removeFromBottom(16));
    xyPad.setBounds(xyArea.reduced(2));

    controlsRow.removeFromLeft(10);
    auto rightCol = controlsRow;
    meter.setBounds(rightCol.removeFromTop(rightCol.getHeight() - 34));
    rightCol.removeFromTop(4);
    randomiseButton.setBounds(rightCol);

    auto presetBarArea = b.removeFromTop(38);
    presetBar.setBounds(presetBarArea.reduced(8, 5));

    viewport.setBounds(b.reduced(8));
}
