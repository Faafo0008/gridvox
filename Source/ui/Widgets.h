#pragma once

#include "Theme.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace ui
{

//==============================================================================
/** Custom rotary control bound directly to an AudioProcessorParameter.
    - vertical drag (Shift = fine)
    - double click = reset to default
    - mouse wheel = small steps
*/
class ParamKnob final : public juce::Component,
                        public juce::SettableTooltipClient
{
public:
    ParamKnob (juce::RangedAudioParameter& parameter, bool bipolarStyle, float diameter);
    ~ParamKnob() override = default;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    /** Places the knob using its centre point (reference coordinates). */
    void setCentreAndDiameter (juce::Point<float> centre, float diameter);

    float getNormalised() const;
    juce::String getValueText() const;

private:
    juce::RangedAudioParameter& param;
    bool bipolar;
    float diameter;

    bool dragging = false;
    float dragStartNormalised = 0.0f;
    float dragDistance = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamKnob)
};

//==============================================================================
/** Vertical segmented level meter with the IN / OUT caption and peak readout. */
class LevelMeter final : public juce::Component
{
public:
    LevelMeter (juce::String caption);
    ~LevelMeter() override = default;

    void paint (juce::Graphics&) override;

    /** level = linear 0..1, applies its own ballistic fall-off. */
    void setLevel (float newLevel);

private:
    juce::String caption;
    float level = 0.0f, displayed = 0.0f, peak = 0.0f;
    int peakHoldCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeter)
};

//==============================================================================
/** Chamfered text button with an obvious selected state. */
class GridButton final : public juce::Component,
                         public juce::SettableTooltipClient
{
public:
    enum class Style { plain, selectedFilled };

    GridButton (juce::String text, Style style = Style::plain);
    ~GridButton() override = default;

    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    void setSelected (bool shouldBeSelected);
    bool isSelected() const noexcept { return selected; }

    std::function<void()> onClick;
    std::function<void (bool)> onSelectionChanged;

private:
    juce::String label;
    Style style;
    bool selected = false, hovered = false, pressed = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GridButton)
};

//==============================================================================
/** Small status line: LED dot + upper-case text. */
class StatusLine final : public juce::Component
{
public:
    StatusLine (juce::String text, juce::Colour ledColour);
    ~StatusLine() override = default;

    void paint (juce::Graphics&) override;
    void setLit (bool shouldBeLit);

private:
    juce::String label;
    juce::Colour colour;
    bool lit = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StatusLine)
};

//==============================================================================
/** Horizontal gain-reduction bar used by the two dynamics modules. */
class ReductionBar final : public juce::Component
{
public:
    ReductionBar() = default;
    ~ReductionBar() override = default;

    void paint (juce::Graphics&) override;
    void setReductionDb (float db, float maxDb = 12.0f);

private:
    float amount = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReductionBar)
};

//==============================================================================
/** Header preset strip: previous / name box (popup) / next. */
class PresetSelector final : public juce::Component,
                             public juce::SettableTooltipClient
{
public:
    PresetSelector();
    ~PresetSelector() override = default;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

    std::function<void (int)> onStep;            // -1 / +1
    std::function<void (int)> onPresetChosen;
    std::function<int()> getPresetIndex;
    std::function<juce::String()> getPresetName;

    void setArrowBounds (juce::Rectangle<float> prev, juce::Rectangle<float> box, juce::Rectangle<float> next);

private:
    juce::Rectangle<float> prevRect, boxRect, nextRect;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetSelector)
};

} // namespace ui
