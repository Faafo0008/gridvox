#include "EditorContent.h"
#include "PluginProcessor.h"
#include "params/Parameters.h"
#include "params/ParameterIDs.h"

using namespace theme;

namespace
{
    // ---- reference-space metrics (all sampled from reference.png) ----------
    constexpr float rowATitleTop  = 74.0f;
    constexpr float rowAValueTop  = 147.0f;
    constexpr float rowCTitleTop  = 349.0f;
    constexpr float rowCValueTop  = 421.0f;

    const float eqGainX[4] = { 84.0f, 202.0f, 320.0f, 436.0f };
    const float eqFreqX[4] = { 137.0f, 254.0f, 372.0f, 488.0f };
    const float eqBandCentre[4] = { 84.0f, 202.0f, 320.0f, 436.0f };

    const juce::Colour dimCyan   { 0xff2c5f6d };
    const juce::Colour midCyan   { 0xff4d8b9b };
    const juce::Colour labelCyan { 0xff7fc2d2 };
    const juce::Colour groupLabel{ 0xff4b9cb0 };
}

//==============================================================================
EditorContent::EditorContent (GridVoxProcessor& p) : processor (p)
{
    theme::initialiseFonts();
    buildChildren();
    setSize ((int) dim::width, (int) dim::height);
}

//==============================================================================
void EditorContent::buildChildren()
{
    auto& apvts = processor.apvts;

    auto addKnob = [this, &apvts] (const char* id, bool bipolar, float diameter) -> ui::ParamKnob*
    {
        if (auto* param = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (id)))
        {
            auto knob = std::make_unique<ui::ParamKnob> (*param, bipolar, diameter);
            addAndMakeVisible (*knob);
            auto* raw = knob.get();
            knobs.push_back (std::move (knob));
            return raw;
        }
        return nullptr;
    };

    gateKnob    = addKnob (ParamIDs::gate, false, 52.0f);
    lowCutKnob  = addKnob (ParamIDs::lowCutFreq, false, 52.0f);
    mudKnob     = addKnob (ParamIDs::mud, false, 52.0f);
    graceKnob   = addKnob (ParamIDs::grace, false, 52.0f);
    fastKnob    = addKnob (ParamIDs::fast, false, 52.0f);
    smoothKnob  = addKnob (ParamIDs::smooth, false, 52.0f);

    for (int i = 0; i < 4; ++i)
    {
        eqGainKnob[i] = addKnob (ParamIDs::eqGain[i], true, 48.0f);
        eqFreqKnob[i] = addKnob (ParamIDs::eqFreq[i], false, 32.0f);
    }

    deEssKnob  = addKnob (ParamIDs::deEss, false, 50.0f);
    warmthKnob = addKnob (ParamIDs::warmth, false, 50.0f);
    reverbKnob = addKnob (ParamIDs::reverbMix, false, 50.0f);
    timeKnob   = addKnob (ParamIDs::reverbTime, false, 32.0f);
    loCutKnob  = addKnob (ParamIDs::reverbLoCut, false, 32.0f);
    hiCutKnob  = addKnob (ParamIDs::reverbHiCut, false, 32.0f);
    outputKnob = addKnob (ParamIDs::output, true, 50.0f);

    // ---- meters ----
    inMeter = std::make_unique<ui::LevelMeter> ("IN");
    outMeter = std::make_unique<ui::LevelMeter> ("OUT");
    addAndMakeVisible (*inMeter);
    addAndMakeVisible (*outMeter);

    // ---- low cut slope buttons ----
    for (int i = 0; i < 3; ++i)
    {
        slopeButtons[i] = std::make_unique<ui::GridButton> (juce::String (12 * (i + 1)), ui::GridButton::Style::plain);
        slopeButtons[i]->setTooltip (juce::String (12 * (i + 1)) + " dB per octave low cut slope");
        addAndMakeVisible (*slopeButtons[i]);

        slopeButtons[i]->onClick = [this, i]
        {
            if (auto* p = processor.apvts.getParameter (ParamIDs::lowCutSlope))
                p->setValueNotifyingHost (p->convertTo0to1 ((float) i));

            for (int j = 0; j < 3; ++j)
                slopeButtons[j]->setSelected (j == i);
        };
    }

    // ---- reverb type buttons ----
    const char* reverbNames[3] = { "ROOM", "PLATE", "HALL" };
    for (int i = 0; i < 3; ++i)
    {
        reverbButtons[i] = std::make_unique<ui::GridButton> (reverbNames[i], ui::GridButton::Style::selectedFilled);
        reverbButtons[i]->setTooltip (juce::String (reverbNames[i]).toLowerCase() + " reverb algorithm");
        addAndMakeVisible (*reverbButtons[i]);

        reverbButtons[i]->onClick = [this, i]
        {
            if (auto* p = processor.apvts.getParameter (ParamIDs::reverbType))
                p->setValueNotifyingHost (p->convertTo0to1 ((float) i));

            for (int j = 0; j < 3; ++j)
                reverbButtons[j]->setSelected (j == i);
        };
    }

    // ---- status lines ----
    rateStatus  = std::make_unique<ui::StatusLine> ("RATING", orange);
    mudStatus   = std::make_unique<ui::StatusLine> ("CLEANING", orange);
    deEssStatus = std::make_unique<ui::StatusLine> ("DE-ESSING", orange);
    limitStatus = std::make_unique<ui::StatusLine> ("LIMITING", orange);
    addAndMakeVisible (*rateStatus);
    addAndMakeVisible (*mudStatus);
    addAndMakeVisible (*deEssStatus);
    addAndMakeVisible (*limitStatus);

    fastBar = std::make_unique<ui::ReductionBar>();
    smoothBar = std::make_unique<ui::ReductionBar>();
    addAndMakeVisible (*fastBar);
    addAndMakeVisible (*smoothBar);

    // ---- header ----
    presetSelector = std::make_unique<ui::PresetSelector>();
    presetSelector->getPresetIndex = [this] { return processor.getCurrentPreset(); };
    presetSelector->getPresetName = [this] { return processor.getCurrentPresetName(); };
    presetSelector->onStep = [this] (int dir)
    {
        if (dir < 0) processor.previousPreset();
        else         processor.nextPreset();
        refreshValues();
    };
    presetSelector->onPresetChosen = [this] (int index)
    {
        processor.setCurrentPreset (index);
        refreshValues();
    };
    addAndMakeVisible (*presetSelector);

    bypassButton = std::make_unique<ui::GridButton> ("BYPASS", ui::GridButton::Style::selectedFilled);
    bypassButton->setTooltip ("Bypass the whole processing chain");
    bypassButton->onClick = [this]
    {
        if (auto* p = processor.apvts.getParameter (ParamIDs::bypass))
            p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f);
    };
    addAndMakeVisible (*bypassButton);
}

//==============================================================================
void EditorContent::resized()
{
    inMeter->setBounds (layout::inMeter.toNearestInt());
    outMeter->setBounds (layout::outMeter.toNearestInt());

    gateKnob->setCentreAndDiameter ({ 78.0f, 114.0f }, 52.0f);
    lowCutKnob->setCentreAndDiameter ({ 146.0f, 114.0f }, 52.0f);
    mudKnob->setCentreAndDiameter ({ 211.0f, 114.0f }, 52.0f);
    graceKnob->setCentreAndDiameter ({ 277.0f, 114.0f }, 52.0f);
    fastKnob->setCentreAndDiameter ({ 365.0f, 114.0f }, 52.0f);
    smoothKnob->setCentreAndDiameter ({ 464.5f, 114.0f }, 52.0f);

    for (int i = 0; i < 4; ++i)
    {
        eqGainKnob[i]->setCentreAndDiameter ({ eqGainX[i], 250.0f }, 48.0f);
        eqFreqKnob[i]->setCentreAndDiameter ({ eqFreqX[i], 254.0f }, 32.0f);
    }

    deEssKnob->setCentreAndDiameter ({ 84.0f, 388.0f }, 50.0f);
    warmthKnob->setCentreAndDiameter ({ 162.0f, 388.0f }, 50.0f);
    reverbKnob->setCentreAndDiameter ({ 234.0f, 386.0f }, 50.0f);
    timeKnob->setCentreAndDiameter ({ 326.0f, 392.0f }, 32.0f);
    loCutKnob->setCentreAndDiameter ({ 370.0f, 392.0f }, 32.0f);
    hiCutKnob->setCentreAndDiameter ({ 413.0f, 392.0f }, 32.0f);
    outputKnob->setCentreAndDiameter ({ 478.0f, 388.0f }, 50.0f);

    for (int i = 0; i < 3; ++i)
        slopeButtons[i]->setBounds (juce::Rectangle<float> (115.0f + (float) i * 21.0f, 167.0f, 19.0f, 12.0f).toNearestInt());

    for (int i = 0; i < 3; ++i)
        reverbButtons[i]->setBounds (juce::Rectangle<float> (266.0f, 360.5f + (float) i * 19.0f, 35.0f, 16.5f).toNearestInt());

    rateStatus->setBounds (juce::Rectangle<float> (54.0f, 159.0f, 52.0f, 10.0f).toNearestInt());
    mudStatus->setBounds (juce::Rectangle<float> (186.0f, 159.0f, 52.0f, 10.0f).toNearestInt());
    deEssStatus->setBounds (juce::Rectangle<float> (56.0f, 438.0f, 58.0f, 10.0f).toNearestInt());
    limitStatus->setBounds (juce::Rectangle<float> (458.0f, 438.0f, 56.0f, 10.0f).toNearestInt());

    fastBar->setBounds (juce::Rectangle<float> (324.0f, 176.0f, 82.0f, 6.0f).toNearestInt());
    smoothBar->setBounds (juce::Rectangle<float> (423.0f, 176.0f, 82.0f, 6.0f).toNearestInt());

    const auto all = layout::prevButton.getUnion (layout::nextButton);
    presetSelector->setBounds (all.expanded (0.0f, 3.0f).toNearestInt());
    presetSelector->setArrowBounds (layout::prevButton.translated (-all.getX(), -all.getY() + 3.0f),
                                    layout::presetBox.translated (-all.getX(), -all.getY() + 3.0f),
                                    layout::nextButton.translated (-all.getX(), -all.getY() + 3.0f));

    bypassButton->setBounds (layout::bypassButton.toNearestInt());
}

//==============================================================================
void EditorContent::centredFitted (juce::Graphics& g, const juce::String& text, float centreX, float top,
                                   float width, float height, Face face, juce::Colour colour)
{
    auto f = fontFitted (face, height, text, width);
    const auto area = juce::Rectangle<float> (centreX - width * 0.5f, top, width, height * 2.2f);

    g.setFont (f);

    // The reference lettering is a heavy geometric face; sub-pixel passes in the
    // four diagonal directions thicken the strokes without smearing the glyphs.
    if (face == Face::label)
    {
        g.setColour (colour.withAlpha (0.8f));
        for (const auto& d : { juce::Point<float> (-0.4f, 0.0f), juce::Point<float> (0.4f, 0.0f),
                               juce::Point<float> (0.0f, -0.4f), juce::Point<float> (0.0f, 0.4f) })
            g.drawText (text, area.translated (d.x, d.y), juce::Justification::centredTop, false);
    }

    g.setColour (colour);
    g.drawText (text, area, juce::Justification::centredTop, false);
}

void EditorContent::centred (juce::Graphics& g, const juce::String& text, float centreX, float top,
                             float width, float height, Face face, juce::Colour colour, float tracking)
{
    drawText (g, text, { centreX - width * 0.5f, top, width, height * 2.0f },
              font (face, height), colour, juce::Justification::centredTop, tracking);
}

void EditorContent::leftText (juce::Graphics& g, const juce::String& text, float x, float top,
                              float width, float height, Face face, juce::Colour colour, float tracking)
{
    drawText (g, text, { x, top, width, height * 2.0f },
              font (face, height), colour, juce::Justification::centredLeft, tracking);
}

juce::String EditorContent::valueOf (ui::ParamKnob* knob) const
{
    return knob != nullptr ? knob->getValueText() : juce::String();
}

//==============================================================================
void EditorContent::paint (juce::Graphics& g)
{
    g.fillAll (windowBg);

    paintHeader (g);
    paintRowA (g);
    paintToneEq (g);
    paintRowC (g);
    paintFooter (g);
    paintTitleBar (g);

    if (processor.apvts.getRawParameterValue (ParamIDs::bypass)->load() > 0.5f)
    {
        g.setColour (juce::Colour (0xff020507).withAlpha (0.55f));
        g.fillRect (juce::Rectangle<float> (10.0f, 58.0f, 540.0f, 414.0f));
        centredFitted (g, "BYPASS", 280.0f, 258.0f, 70.0f, 13.0f, Face::label, cyan.withAlpha (0.8f));
    }
}

//==============================================================================
void EditorContent::paintTitleBar (juce::Graphics& g)
{
    auto bar = juce::Rectangle<float> (0.0f, 0.0f, dim::width, dim::titleH);
    g.setColour (titleBarBg);
    g.fillRect (bar);
    g.setColour (juce::Colour (0xff3c3c3c));
    g.fillRect (0.0f, 0.0f, dim::width, 1.0f);
    g.setColour (juce::Colour (0xff141414));
    g.fillRect (0.0f, dim::titleH - 1.0f, dim::width, 1.0f);

    // transport / settings glyphs
    {
        juce::Path play;
        play.addTriangle (8.5f, 6.2f, 8.5f, 12.6f, 13.0f, 9.4f);
        g.setColour (green.withAlpha (0.75f));
        g.fillPath (play);

        // gear: outlined ring with square-ish teeth, as in the artwork
        const juce::Point<float> c (26.5f, 9.4f);
        juce::Path gear;
        constexpr int teeth = 8;
        for (int i = 0; i < teeth * 2; ++i)
        {
            const float a = (float) i / (float) (teeth * 2) * juce::MathConstants<float>::twoPi;
            const float r = (i % 2 == 0) ? 5.2f : 3.9f;
            const juce::Point<float> p (c.x + r * std::sin (a), c.y - r * std::cos (a));
            if (i == 0) gear.startNewSubPath (p);
            else        gear.lineTo (p);
        }
        gear.closeSubPath();
        g.setColour (green.withAlpha (0.9f));
        g.strokePath (gear, juce::PathStrokeType (1.0f));
        g.fillEllipse (c.x - 1.5f, c.y - 1.5f, 3.0f, 3.0f);
    }

    // title text
    {
        auto f = fontFitted (Face::mono, 9.4f, "GRID VOX", 43.0f);
        g.setFont (f);
        const juce::Rectangle<float> area (39.0f, 4.0f, 43.0f, 11.0f);
        g.setColour (green.withAlpha (0.55f));
        g.drawText ("GRID VOX", area.translated (0.4f, 0.0f), juce::Justification::centredLeft, false);
        g.setColour (green);
        g.drawText ("GRID VOX", area, juce::Justification::centredLeft, false);

        auto f2 = fontFitted (Face::value, 8.6f, "(Insert 2)", 37.0f);
        g.setFont (f2);
        g.setColour (green.withAlpha (0.85f));
        g.drawText ("(Insert 2)", juce::Rectangle<float> (85.0f, 4.0f, 38.0f, 11.0f),
                    juce::Justification::centredLeft, false);
    }

    // grid glyph (3 x 3 mini patch bay)
    {
        const float x0 = 442.0f, y0 = 5.0f, cell = 2.3f, gap = 1.1f;
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 3; ++col)
            {
                g.setColour (green.withAlpha (0.35f + 0.16f * (float) (row + col)));
                g.drawRect (x0 + (float) col * (cell + gap), y0 + (float) row * (cell + gap), cell, cell, 0.8f);
            }
    }

    {
        const auto f = fontFitted (Face::value, 9.0f, "Presets", 32.0f);
        drawText (g, "Presets", { 464.0f, 4.0f, 32.0f, 11.0f }, f, green, juce::Justification::centredLeft, 0.04f);
    }

    // preset arrows
    {
        juce::Path prev, next;
        prev.addTriangle (506.0f, 6.2f, 506.0f, 12.6f, 502.2f, 9.4f);
        next.addTriangle (515.0f, 6.2f, 515.0f, 12.6f, 518.8f, 9.4f);
        g.setColour (green.withAlpha (0.9f));
        g.fillPath (prev);
        g.setColour (green);
        g.fillPath (next);
    }

    // window controls
    g.setColour (green.withAlpha (0.85f));
    g.drawLine (533.5f, 9.4f, 539.5f, 9.4f, 1.6f);
    {
        juce::Path close;
        close.startNewSubPath (547.5f, 5.6f); close.lineTo (555.5f, 13.2f);
        close.startNewSubPath (555.5f, 5.6f); close.lineTo (547.5f, 13.2f);
        g.strokePath (close, juce::PathStrokeType (1.4f));
    }
}

//==============================================================================
void EditorContent::paintHeader (juce::Graphics& g)
{
    auto header = juce::Rectangle<float> (0.0f, dim::titleH, dim::width, 38.0f);
    juce::ColourGradient grad (juce::Colour (0xff050b0e), 0.0f, header.getY(),
                               juce::Colour (0xff02060a), 0.0f, header.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRect (header);

    auto drawLogoPart = [&] (const juce::String& text, float x, float width)
    {
        auto f = fontFitted (Face::logo, 21.0f, text, width);
        g.setFont (f);
        const auto area = juce::Rectangle<float> (x, 31.5f, width + 2.0f, 22.0f);

        g.setColour (cyan.withAlpha (0.16f));
        g.drawText (text, area.translated (-0.8f, 0.0f), juce::Justification::centredLeft, false);
        g.drawText (text, area.translated (0.8f, 0.0f), juce::Justification::centredLeft, false);
        g.setColour (juce::Colour (0xffd8fbff));
        g.drawText (text, area, juce::Justification::centredLeft, false);
    };

    drawLogoPart ("GRID", 19.0f, 52.0f);
    drawLogoPart ("VOX", 84.0f, 48.0f);

    {
        auto f1 = fontFitted (Face::label, 6.2f, "VOCAL TRACKING CHAIN", 88.0f);
        g.setFont (f1);
        g.setColour (juce::Colour (0xff3c8296));
        g.drawText ("VOCAL TRACKING CHAIN", juce::Rectangle<float> (138.0f, 34.0f, 88.0f, 8.0f),
                    juce::Justification::centredLeft, false);

        auto f2 = fontFitted (Face::label, 6.0f, "ZERO LATENCY", 56.0f);
        g.setFont (f2);
        g.setColour (juce::Colour (0xff2d6474));
        g.drawText ("ZERO LATENCY", juce::Rectangle<float> (138.0f, 42.5f, 62.0f, 8.0f),
                    juce::Justification::centredLeft, false);
    }

    g.setColour (juce::Colour (0xff0a171c));
    g.fillRect (0.0f, 55.0f, dim::width, 1.0f);
}

//==============================================================================
void EditorContent::paintRowA (juce::Graphics& g)
{
    drawLabelledPanel (g, layout::gate, Corners::modules(), "CLEAN-UP", 52.0f, 7.0f, groupLabel);
    drawPanel (g, layout::lowCut, Corners::modules());
    drawPanel (g, layout::mud, Corners::modules());
    drawPanel (g, layout::grace, Corners::modules());
    drawLabelledPanel (g, layout::dynamics, Corners::modules(), "DYNAMICS", 331.0f, 7.0f, groupLabel);

    // ---- titles ----
    centredFitted (g, "GATE",    78.0f,  rowATitleTop, 23.5f, 7.8f, Face::label, titleText);
    centredFitted (g, "LOW CUT", 146.0f, rowATitleTop, 45.0f, 7.8f, Face::label, titleText);
    centredFitted (g, "MUD",     211.0f, rowATitleTop, 22.5f, 7.8f, Face::label, titleText);
    centredFitted (g, "GRACE",   277.0f, rowATitleTop, 34.0f, 7.8f, Face::label, titleText);
    centredFitted (g, "FAST",    365.0f, rowATitleTop, 28.0f, 7.8f, Face::label, titleText);
    centredFitted (g, "SMOOTH",  464.5f, rowATitleTop, 41.0f, 7.8f, Face::label, titleText);

    // ---- values ----
    centredFitted (g, valueOf (gateKnob),   78.0f,   rowAValueTop, 22.0f, 9.0f, Face::value, valueText);
    centredFitted (g, valueOf (lowCutKnob), 146.0f,  rowAValueTop, 30.0f, 9.0f, Face::value, valueText);
    centredFitted (g, valueOf (mudKnob),    211.0f,  rowAValueTop, 22.0f, 9.0f, Face::value, valueText);
    centredFitted (g, valueOf (graceKnob),  277.0f,  rowAValueTop, 22.0f, 9.0f, Face::value, valueText);
    centredFitted (g, valueOf (fastKnob),   365.0f,  rowAValueTop, 22.0f, 9.0f, Face::value, valueText);
    centredFitted (g, valueOf (smoothKnob), 464.5f,  rowAValueTop, 22.0f, 9.0f, Face::value, valueText);

    centredFitted (g, "PRO CURVE", 277.0f, 179.0f, 46.0f, 6.4f, Face::caption, dimCyan);

    // ---- dynamics ----
    centredFitted (g, "PEAK CONTROL",    365.0f, 157.5f, 56.0f, 6.2f, Face::caption, dimCyan);
    centredFitted (g, "GENTLE LEVELING", 464.5f, 157.5f, 66.0f, 6.2f, Face::caption, dimCyan);

    centredFitted (g, "FAST",   336.0f, 168.0f, 22.0f, 6.2f, Face::caption, dimCyan);
    centredFitted (g, "SMOOTH", 440.0f, 168.0f, 34.0f, 6.2f, Face::caption, dimCyan);
    centredFitted (g, "0.0 DB", 392.0f, 168.0f, 26.0f, 6.2f, Face::caption, dimCyan);
    centredFitted (g, "0.0 DB", 491.5f, 168.0f, 26.0f, 6.2f, Face::caption, dimCyan);
}

//==============================================================================
void EditorContent::paintToneEq (juce::Graphics& g)
{
    drawLabelledPanel (g, layout::toneEq, Corners::group(), "TONE EQ", 56.0f, 7.0f, groupLabel);

    for (int i = 1; i < 4; ++i)
    {
        const float x = layout::toneEq.getX() + 117.0f * (float) i;
        g.setColour (juce::Colour (0xff0e2f3a));
        g.fillRect (x, layout::toneEq.getY() + 10.0f, 1.0f, layout::toneEq.getHeight() - 20.0f);
    }

    for (int i = 0; i < 4; ++i)
    {
        centredFitted (g, Params::eqBandNames[i], eqBandCentre[i], 212.0f, 33.0f, 7.8f, Face::label, titleText);
        centredFitted (g, "FREQ", eqFreqX[i], 228.5f, 24.0f, 8.0f, Face::label, labelCyan.withAlpha (0.72f));

        // Fixed-size readouts: the artwork keeps a constant type size across the
        // four bands rather than fitting each string to its column.
        drawText (g, valueOf (eqFreqKnob[i]), { eqFreqX[i] - 30.0f, 271.5f, 60.0f, 8.0f },
                  font (Face::value, 7.2f), midCyan, juce::Justification::centred, 0.04f);
        drawText (g, valueOf (eqGainKnob[i]), { eqBandCentre[i] - 30.0f, 282.5f, 60.0f, 8.0f },
                  font (Face::value, 7.2f), juce::Colour (0xff42899a), juce::Justification::centred, 0.04f);

        centredFitted (g, Params::eqBandCharacter[i], eqBandCentre[i], 310.0f, 41.0f, 6.4f, Face::caption, dimCyan);
    }
}

//==============================================================================
void EditorContent::paintRowC (juce::Graphics& g)
{
    drawPanel (g, layout::deEss, Corners::modules());
    drawPanel (g, layout::warmth, Corners::modules());
    drawLabelledPanel (g, layout::space, Corners::modules(), "SPACE", 211.0f, 7.0f, groupLabel);
    drawPanel (g, layout::output, Corners::modules());

    centredFitted (g, "DE-ESS", 84.0f,  rowCTitleTop, 33.0f, 7.8f, Face::label, titleText);
    centredFitted (g, "WARMTH", 162.0f, rowCTitleTop, 42.5f, 7.8f, Face::label, titleText);
    centredFitted (g, "REVERB", 234.0f, rowCTitleTop, 40.0f, 7.8f, Face::label, titleText);
    centredFitted (g, "OUTPUT", 478.0f, rowCTitleTop, 39.0f, 7.8f, Face::label, titleText);

    centredFitted (g, valueOf (deEssKnob),  84.0f,  rowCValueTop, 22.0f, 9.0f, Face::value, valueText);
    centredFitted (g, valueOf (warmthKnob), 162.0f, rowCValueTop, 22.0f, 9.0f, Face::value, valueText);
    centredFitted (g, valueOf (reverbKnob), 234.0f, rowCValueTop, 22.0f, 9.0f, Face::value, valueText);
    centredFitted (g, valueOf (outputKnob), 478.0f, rowCValueTop, 36.0f, 9.0f, Face::value, valueText);

    centredFitted (g, "CHEST NO MUD", 152.0f, 441.0f, 42.0f, 6.2f, Face::caption, dimCyan);

    // ---- space ----
    centredFitted (g, "TIME",   326.0f, 366.5f, 18.0f, 6.6f, Face::caption, midCyan);
    centredFitted (g, "LO CUT", 370.0f, 366.5f, 30.0f, 6.6f, Face::caption, midCyan);
    centredFitted (g, "HI CUT", 413.0f, 366.5f, 27.0f, 6.6f, Face::caption, midCyan);

    centredFitted (g, valueOf (timeKnob),  326.0f, 394.5f, 28.0f, 8.0f, Face::value, midCyan);
    centredFitted (g, valueOf (loCutKnob), 370.0f, 394.5f, 36.0f, 8.0f, Face::value, midCyan);
    centredFitted (g, valueOf (hiCutKnob), 413.0f, 394.5f, 34.0f, 8.0f, Face::value, midCyan);

    centredFitted (g, processor.getTempoDisplay(), 320.0f, 443.0f, 108.0f, 7.6f, Face::mono,
                   juce::Colour (0xff4b93a4));
}

//==============================================================================
void EditorContent::paintFooter (juce::Graphics& g)
{
    auto badge = [&g] (juce::Rectangle<float> r, const juce::String& text)
    {
        const auto path = chamferPath (r, Corners::small (4.0f));
        g.setColour (juce::Colour (0xff061218));
        g.fillPath (path);
        g.setColour (juce::Colour (0xff194e5d));
        g.strokePath (path, juce::PathStrokeType (0.9f));
        drawText (g, text, r, fontFitted (Face::mono, 7.6f, text, r.getWidth() - 12.0f),
                  juce::Colour (0xff6dbccd), juce::Justification::centred, 0.05f);
    };

    const auto latencyText = "LATENCY " + juce::String (gv::VocalChain::getLatencySamples()) + " SAMPLES";
    badge (layout::latencyBadge, latencyText);
    badge (layout::licensedBadge, "LICENSED");

    drawText (g, "DOUBLE-CLICK = RESET", { 135.0f, 480.0f, 88.0f, 8.0f }, font (Face::mono, 7.0f), dimCyan,
              juce::Justification::centredLeft, 0.04f);
    drawText (g, "SHIFT-DRAG = FINE", { 232.0f, 480.0f, 72.0f, 8.0f }, font (Face::mono, 7.0f), dimCyan,
              juce::Justification::centredLeft, 0.04f);
    drawText (g, "DRAG CORNER = RESIZE", { 312.0f, 480.0f, 90.0f, 8.0f }, font (Face::mono, 7.0f), dimCyan,
              juce::Justification::centredLeft, 0.04f);

    g.setColour (juce::Colour (0xff0d2831));
    g.fillRect (0.0f, dim::height - 3.0f, dim::width, 1.0f);

    g.setColour (juce::Colour (0xff1b4854));
    for (int i = 0; i < 4; ++i)
    {
        const float offset = 5.0f + (float) i * 4.0f;
        g.drawLine (dim::width - 2.0f - offset, dim::height - 2.0f, dim::width - 2.0f, dim::height - 2.0f - offset, 1.0f);
        g.drawLine (2.0f + offset, dim::height - 2.0f, 2.0f, dim::height - 2.0f - offset, 1.0f);
    }
}

//==============================================================================
void EditorContent::refresh (const UiStatus& s)
{
    inMeter->setLevel (s.inputPeak);
    outMeter->setLevel (s.outputPeak);

    rateStatus->setLit (s.gateActive);
    mudStatus->setLit (s.mudActive);
    deEssStatus->setLit (s.deEssActive);
    limitStatus->setLit (s.limiterActive);

    fastBar->setReductionDb (s.fastReductionDb, 12.0f);
    smoothBar->setReductionDb (s.smoothReductionDb, 12.0f);
}

void EditorContent::refreshValues()
{
    if (auto* p = processor.apvts.getRawParameterValue (ParamIDs::lowCutSlope))
    {
        const int index = juce::roundToInt (p->load());
        for (int i = 0; i < 3; ++i)
            slopeButtons[i]->setSelected (i == index);
    }

    if (auto* p = processor.apvts.getRawParameterValue (ParamIDs::reverbType))
    {
        const int index = juce::roundToInt (p->load());
        for (int i = 0; i < 3; ++i)
            reverbButtons[i]->setSelected (i == index);
    }

    if (auto* p = processor.apvts.getRawParameterValue (ParamIDs::bypass))
        bypassButton->setSelected (p->load() > 0.5f);

    for (auto& k : knobs)
        k->repaint();

    repaint();
}
