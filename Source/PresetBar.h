#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PresetManager.h"
#include "PhatLookAndFeel.h"

// The preset strip: << prev | preset combo | next >> | SAVE | DELETE.
// Listens to the PresetManager's ChangeBroadcaster so it stays in sync
// whenever a preset is saved, deleted, or stepped through from code (or
// after a DAW session reload restores a preset name).
class PresetBar : public juce::Component, private juce::ChangeListener
{
public:
    explicit PresetBar(PresetManager& pm) : presetManager(pm)
    {
        prevButton.setButtonText("<");
        nextButton.setButtonText(">");
        saveButton.setButtonText("SAVE");
        deleteButton.setButtonText("DEL");

        for (auto* b : { &prevButton, &nextButton, &saveButton, &deleteButton })
            addAndMakeVisible(b);

        addAndMakeVisible(presetBox);

        prevButton.onClick   = [this] { presetManager.loadPreviousPreset(); };
        nextButton.onClick   = [this] { presetManager.loadNextPreset(); };
        saveButton.onClick   = [this] { showSaveDialog(); };
        deleteButton.onClick = [this] { showDeleteConfirm(); };

        presetBox.onChange = [this]
        {
            auto name = presetBox.getItemText(presetBox.getSelectedItemIndex());
            if (name.isNotEmpty() && name != presetManager.getCurrentPresetName())
                presetManager.loadPreset(name);
        };

        presetManager.addChangeListener(this);
        refreshList();
    }

    ~PresetBar() override
    {
        presetManager.removeChangeListener(this);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        prevButton.setBounds(b.removeFromLeft(28).reduced(2));
        nextButton.setBounds(b.removeFromRight(28).reduced(2));
        deleteButton.setBounds(b.removeFromRight(52).reduced(2));
        saveButton.setBounds(b.removeFromRight(60).reduced(2));
        b.removeFromRight(6);
        presetBox.setBounds(b.reduced(2));
    }

private:
    void refreshList()
    {
        presetBox.clear(juce::dontSendNotification);
        auto names = presetManager.getAllPresetNames();
        presetBox.addItemList(names, 1);

        auto current = presetManager.getCurrentPresetName();
        auto idx = names.indexOf(current);
        if (idx >= 0)
            presetBox.setSelectedItemIndex(idx, juce::dontSendNotification);
        else
            presetBox.setText(current, juce::dontSendNotification);
    }

    void showSaveDialog()
    {
        auto* aw = new juce::AlertWindow("Save Preset", "Name this preset:", juce::MessageBoxIconType::NoIcon);
        aw->addTextEditor("name", presetManager.getCurrentPresetName());
        aw->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
        aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

        aw->enterModalState(true, juce::ModalCallbackFunction::create([this, aw](int result)
        {
            if (result == 1)
            {
                auto name = aw->getTextEditorContents("name").trim();
                if (name.isNotEmpty())
                    presetManager.savePreset(name);
            }
        }), true);
    }

    void showDeleteConfirm()
    {
        auto name = presetManager.getCurrentPresetName();
        auto* aw = new juce::AlertWindow("Delete Preset",
                                          "Delete \"" + name + "\" permanently?",
                                          juce::MessageBoxIconType::WarningIcon);
        aw->addButton("Delete", 1, juce::KeyPress(juce::KeyPress::returnKey));
        aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

        aw->enterModalState(true, juce::ModalCallbackFunction::create([this, name](int result)
        {
            if (result == 1)
                presetManager.deletePreset(name);
        }), true);
    }

    void changeListenerCallback(juce::ChangeBroadcaster*) override { refreshList(); }

    PresetManager& presetManager;
    juce::ComboBox presetBox;
    juce::TextButton prevButton, nextButton, saveButton, deleteButton;
};
