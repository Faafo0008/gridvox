#pragma once

#include "Theme.h"
#include "Widgets.h"
#include "UiStatus.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <vector>

class GridVoxProcessor;

/*  The complete 560 x 499 interface. Everything is laid out in reference
    pixels; the plugin editor scales this component as a whole.
*/
class EditorContent final : public juce::Component
{
public:
    explicit EditorContent (GridVoxProcessor&);
    ~EditorContent() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

    /** Called from the editor timer (30 Hz). */
    void refresh (const UiStatus&);
    void refreshValues();

private:
    void buildChildren();
    void paintTitleBar (juce::Graphics&);
    void paintHeader (juce::Graphics&);
    void paintRowA (juce::Graphics&);
    void paintToneEq (juce::Graphics&);
    void paintRowC (juce::Graphics&);
    void paintFooter (juce::Graphics&);

    // text helpers -----------------------------------------------------------
    void centredFitted (juce::Graphics&, const juce::String& text, float centreX, float top,
                        float width, float height, theme::Face face, juce::Colour colour);
    void centred (juce::Graphics&, const juce::String& text, float centreX, float top,
                  float width, float height, theme::Face face, juce::Colour colour, float tracking = 0.0f);
    void leftText (juce::Graphics&, const juce::String& text, float x, float top,
                   float width, float height, theme::Face face, juce::Colour colour, float tracking = 0.0f);

    juce::String valueOf (ui::ParamKnob* knob) const;

    GridVoxProcessor& processor;

    std::vector<std::unique_ptr<ui::ParamKnob>> knobs;
    std::vector<std::unique_ptr<ui::GridButton>> buttons;
    std::unique_ptr<ui::LevelMeter> inMeter, outMeter;
    std::unique_ptr<ui::StatusLine> rateStatus, mudStatus, deEssStatus, limitStatus;
    std::unique_ptr<ui::ReductionBar> fastBar, smoothBar;
    std::unique_ptr<ui::PresetSelector> presetSelector;
    std::unique_ptr<ui::GridButton> bypassButton, slopeButtons[3], reverbButtons[3];

    // named knob handles (owned by the knobs vector)
    ui::ParamKnob* gateKnob = nullptr;
    ui::ParamKnob* lowCutKnob = nullptr;
    ui::ParamKnob* mudKnob = nullptr;
    ui::ParamKnob* graceKnob = nullptr;
    ui::ParamKnob* fastKnob = nullptr;
    ui::ParamKnob* smoothKnob = nullptr;
    ui::ParamKnob* eqGainKnob[4] {};
    ui::ParamKnob* eqFreqKnob[4] {};
    ui::ParamKnob* deEssKnob = nullptr;
    ui::ParamKnob* warmthKnob = nullptr;
    ui::ParamKnob* reverbKnob = nullptr;
    ui::ParamKnob* timeKnob = nullptr;
    ui::ParamKnob* loCutKnob = nullptr;
    ui::ParamKnob* hiCutKnob = nullptr;
    ui::ParamKnob* outputKnob = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EditorContent)
};
