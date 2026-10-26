#include "Widgets.h"
#include "presets/Presets.h"

namespace ui
{

//==============================================================================
ParamKnob::ParamKnob (juce::RangedAudioParameter& parameter, bool bipolarStyle, float d)
    : param (parameter), bipolar (bipolarStyle), diameter (d)
{
    setSize ((int) std::ceil (d), (int) std::ceil (d));
    setRepaintsOnMouseActivity (true);
    setTooltip (param.getName (100));
}

void ParamKnob::setCentreAndDiameter (juce::Point<float> centre, float d)
{
    diameter = d;
    setBounds (juce::Rectangle<float> (d, d).withCentre (centre).toNearestInt());
}

float ParamKnob::getNormalised() const
{
    // AudioProcessorParameter::getValue() is already normalised (0..1).
    return juce::jlimit (0.0f, 1.0f, param.getValue());
}

juce::String ParamKnob::getValueText() const
{
    return param.getCurrentValueAsText().toUpperCase();
}

void ParamKnob::paint (juce::Graphics& g)
{
    const auto t = juce::jlimit (0.0f, 1.0f, getNormalised());
    theme::drawKnob (g, getLocalBounds().toFloat(), t, bipolar);

    if (isMouseOverOrDragging())
    {
        g.setColour (theme::cyan.withAlpha (0.07f));
        g.fillEllipse (getLocalBounds().toFloat().reduced (1.0f));
    }
}

void ParamKnob::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu menu;
        menu.addSectionHeader (param.getName (48));
        menu.addItem (1, "Reset to default (" + param.getCurrentValueAsText().toUpperCase() + ")");
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                            [this] (int result)
                            {
                                if (result == 1)
                                {
                                    param.beginChangeGesture();
                                    param.setValueNotifyingHost (param.getDefaultValue());
                                    param.endChangeGesture();
                                    repaint();
                                }
                            });
        return;
    }

    dragging = true;
    dragStartNormalised = getNormalised();
    dragDistance = 0.0f;
    param.beginChangeGesture();
}

void ParamKnob::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging)
        return;

    const float sensitivity = e.mods.isShiftDown() ? 0.18f : 1.0f;
    dragDistance -= (float) e.getDistanceFromDragStartY();
    auto delta = dragDistance / 150.0f * sensitivity;

    const auto newValue = juce::jlimit (0.0f, 1.0f, dragStartNormalised + delta);
    param.setValueNotifyingHost (newValue);
    repaint();
}

void ParamKnob::mouseUp (const juce::MouseEvent&)
{
    if (dragging)
    {
        dragging = false;
        param.endChangeGesture();
    }
}

void ParamKnob::mouseDoubleClick (const juce::MouseEvent&)
{
    param.beginChangeGesture();
    param.setValueNotifyingHost (param.getDefaultValue());
    param.endChangeGesture();
    repaint();
}

void ParamKnob::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    const float step = (wheel.isReversed ? -1.0f : 1.0f) * (std::abs (wheel.deltaY) > 0.05f ? 0.01f : 0.002f);
    param.beginChangeGesture();
    param.setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, getNormalised() + (wheel.deltaY > 0 ? step : -step)));
    param.endChangeGesture();
    repaint();
}

//==============================================================================
LevelMeter::LevelMeter (juce::String c) : caption (std::move (c))
{
    setInterceptsMouseClicks (false, false);
}

void LevelMeter::setLevel (float newLevel)
{
    const float target = juce::jlimit (0.0f, 1.0f, newLevel);

    // Fast attack, smooth release so the meter behaves like a real peak meter.
    displayed = target > displayed ? target : displayed * 0.86f + target * 0.14f;

    if (target > peak)
    {
        peak = target;
        peakHoldCounter = 45;
    }
    else if (--peakHoldCounter <= 0)
    {
        peak *= 0.995f;
    }

    repaint();
}

void LevelMeter::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    theme::drawPanel (g, bounds, theme::Corners::modules());

    const auto inner = bounds.reduced (3.0f, 0.0f);

    // caption --------------------------------------------------------------
    theme::drawText (g, caption, { inner.getX(), bounds.getY() + 8.0f, inner.getWidth(), 8.0f },
                     theme::font (theme::Face::mono, 7.4f), juce::Colour (0xffb6e2ec),
                     juce::Justification::centred, 0.02f);

    // over-load zone -------------------------------------------------------
    auto clip = juce::Rectangle<float> (inner.getX(), bounds.getY() + 17.0f, inner.getWidth(), 7.0f);
    g.setColour (displayed > 0.985f ? juce::Colour (0xff8c2a18) : juce::Colour (0xff2a1410));
    g.fillRect (clip);

    // segment ladder -------------------------------------------------------
    auto track = juce::Rectangle<float> (inner.getX(), bounds.getY() + 30.0f,
                                        inner.getWidth(), bounds.getHeight() - 34.0f);

    constexpr int columns = 2;
    constexpr int rows = 37;
    const float rowH = track.getHeight() / (float) rows;
    const float colW = (track.getWidth() + 2.0f) / (float) columns;

    g.setColour (juce::Colour (0xff071c24));
    g.fillRect (track);

    // dark grid: 1 px gaps between rows, 2 px between the two columns
    g.setColour (juce::Colour (0xff04121a));
    for (int r = 1; r < rows; ++r)
        g.fillRect (track.getX(), track.getY() + (float) r * rowH, track.getWidth(), 1.1f);
    g.fillRect (track.getX() + colW - 2.0f, track.getY(), 2.0f, track.getHeight());

    // lit segments, bottom-up
    const int litRows = juce::roundToInt (juce::jlimit (0.0f, 1.0f, displayed) * (float) rows);
    for (int r = 0; r < litRows; ++r)
    {
        const float y = track.getBottom() - (float) (r + 1) * rowH;
        const float frac = (float) r / (float) rows;
        const bool hot = frac > 0.93f;
        const auto c = (hot ? theme::orange : theme::cyan).withAlpha (hot ? 0.85f : (0.30f + 0.6f * frac));
        g.setColour (c);
        for (int col = 0; col < columns; ++col)
            g.fillRect (track.getX() + (float) col * colW, y, colW - (col == columns - 1 ? 0.0f : 2.0f), rowH - 1.1f);
    }

    // the top three segments are the hot zone and stay tinted red when idle
    for (int r = rows - 3; r < rows; ++r)
        if (r >= litRows)
        {
            const float y = track.getBottom() - (float) (r + 1) * rowH;
            g.setColour (juce::Colour (0xff33140f));
            g.fillRect (track.getX(), y, track.getWidth(), rowH - 1.1f);
        }

    // peak marker
    if (peak > 0.01f)
    {
        const float y = juce::jlimit (track.getY(), track.getBottom() - 1.0f,
                                      track.getBottom() - peak * track.getHeight());
        g.setColour (theme::cyanHot.withAlpha (0.9f));
        g.fillRect (track.getX(), y, track.getWidth(), 1.0f);
    }

    // readout --------------------------------------------------------------
    const auto db = displayed <= 0.0005f ? juce::String ("-INF")
                                         : juce::String (20.0f * std::log10 (displayed), 1);
    theme::drawText (g, db, { inner.getX(), bounds.getBottom() - 15.0f, inner.getWidth(), 8.0f },
                     theme::font (theme::Face::mono, 7.2f), juce::Colour (0xff3f7d8c),
                     juce::Justification::centred, 0.02f);
}

//==============================================================================
GridButton::GridButton (juce::String text, Style s) : label (std::move (text)), style (s) {}

void GridButton::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    theme::drawButtonFace (g, b, selected, hovered || pressed);

    const auto textColour = selected ? juce::Colour (0xffe2f6fb)
                                     : (hovered ? juce::Colour (0xffc6d2d7) : juce::Colour (0xff9aa6ab));

    const float maxHeight = style == Style::plain ? juce::jmin (7.0f, b.getHeight() * 0.48f)
                                                  : juce::jmin (9.2f, b.getHeight() * 0.58f);

    // Fit the label inside the rounded face including the letter tracking, so
    // nothing is ever clipped on the narrow buttons.
    const float maxWidth = b.getWidth() - 8.0f;
    constexpr float tracking = 0.12f;

    float height = maxHeight;
    auto make = [&] (float h) { return theme::font (theme::Face::value, h).withExtraKerningFactor (tracking); };
    auto f = make (height);
    for (int i = 0; i < 24 && f.getStringWidthFloat (label) > maxWidth && height > 3.6f; ++i)
    {
        height -= 0.25f;
        f = make (height);
    }

    theme::drawText (g, label, b, f, textColour, juce::Justification::centred, 0.0f);
}

void GridButton::mouseEnter (const juce::MouseEvent&) { hovered = true; repaint(); }
void GridButton::mouseExit (const juce::MouseEvent&)  { hovered = false; pressed = false; repaint(); }

void GridButton::mouseDown (const juce::MouseEvent&) { pressed = true; repaint(); }

void GridButton::mouseUp (const juce::MouseEvent& e)
{
    pressed = false;
    repaint();

    if (getLocalBounds().toFloat().contains (e.position))
    {
        if (onClick)
            onClick();
    }
}

void GridButton::setSelected (bool shouldBeSelected)
{
    if (selected == shouldBeSelected)
        return;

    selected = shouldBeSelected;
    repaint();

    if (onSelectionChanged)
        onSelectionChanged (selected);
}

//==============================================================================
StatusLine::StatusLine (juce::String text, juce::Colour c) : label (std::move (text)), colour (c)
{
    setInterceptsMouseClicks (false, false);
}

void StatusLine::setLit (bool shouldBeLit)
{
    if (lit == shouldBeLit)
        return;

    lit = shouldBeLit;
    repaint();
}

void StatusLine::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const auto dot = juce::Point<float> (b.getX() + 3.5f, b.getCentreY());
    theme::drawLed (g, dot, 1.9f, colour, lit);

    theme::drawText (g, label, b.withTrimmedLeft (9.0f), theme::font (theme::Face::mono, 8.0f),
                     lit ? colour.withMultipliedSaturation (0.55f).brighter (0.15f) : theme::mutedText,
                     juce::Justification::centredLeft, 0.06f);
}

//==============================================================================
void ReductionBar::setReductionDb (float db, float maxDb)
{
    const float target = juce::jlimit (0.0f, 1.0f, db / juce::jmax (0.1f, maxDb));
    if (std::abs (target - amount) < 0.004f)
        return;

    amount = target;
    repaint();
}

void ReductionBar::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff0a161c));
    g.fillRect (b);

    const int segments = 28;
    const float segW = b.getWidth() / (float) segments;
    const int lit = juce::roundToInt (amount * (float) segments);

    for (int i = 0; i < segments; ++i)
    {
        const float x = b.getX() + (float) i * segW;
        const bool on = i < lit;
        g.setColour (on ? theme::orange.withAlpha (0.9f) : juce::Colour (0xff1b0c06));
        g.fillRect (x, b.getY(), juce::jmax (1.0f, segW - 1.0f), b.getHeight());
    }
}

//==============================================================================
PresetSelector::PresetSelector()
{
    setInterceptsMouseClicks (true, false);
    setTooltip ("Click to choose a factory preset");
}

void PresetSelector::setArrowBounds (juce::Rectangle<float> prev, juce::Rectangle<float> box, juce::Rectangle<float> next)
{
    prevRect = prev;
    boxRect = box;
    nextRect = next;
}

void PresetSelector::paint (juce::Graphics& g)
{
    // ---- previous / next arrow buttons ----
    for (int i = 0; i < 2; ++i)
    {
        const auto r = i == 0 ? prevRect : nextRect;
        const auto path = theme::chamferPath (r, theme::Corners::small (3.0f));
        g.setColour (juce::Colour (0xff0a151b));
        g.fillPath (path);
        g.setColour (theme::border);
        g.strokePath (path, juce::PathStrokeType (0.9f));

        const float cx = r.getCentreX(), cy = r.getCentreY();
        const float dir = i == 0 ? -1.0f : 1.0f;
        juce::Path arrow;
        arrow.startNewSubPath (cx + dir * -2.2f, cy - 3.4f);
        arrow.lineTo (cx + dir * 2.2f, cy);
        arrow.lineTo (cx + dir * -2.2f, cy + 3.4f);
        g.setColour (theme::cyan);
        g.strokePath (arrow, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // ---- preset name box ----
    {
        const auto path = theme::chamferPath (boxRect, theme::Corners::small (4.0f));
        juce::ColourGradient grad (juce::Colour (0xff0d2831), boxRect.getCentreX(), boxRect.getY(),
                                   juce::Colour (0xff061218), boxRect.getCentreX(), boxRect.getBottom(), false);
        g.setGradientFill (grad);
        g.fillPath (path);
        g.setColour (theme::border);
        g.strokePath (path, juce::PathStrokeType (1.0f));
    }

    const auto name = getPresetName ? getPresetName() : juce::String ("CLEAN TRACKING");
    theme::drawText (g, name, boxRect.withTrimmedLeft (8.0f).withTrimmedRight (22.0f),
                     theme::font (theme::Face::captionBold, 8.6f), juce::Colours::white.withAlpha (0.86f),
                     juce::Justification::centred, 0.22f);

    // chevron
    {
        const float cx = boxRect.getRight() - 12.0f, cy = boxRect.getCentreY();
        juce::Path chev;
        chev.startNewSubPath (cx - 3.2f, cy - 1.6f);
        chev.lineTo (cx, cy + 1.8f);
        chev.lineTo (cx + 3.2f, cy - 1.6f);
        g.setColour (theme::cyan);
        g.strokePath (chev, juce::PathStrokeType (1.1f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

void PresetSelector::mouseDown (const juce::MouseEvent& e)
{
    const auto p = e.position;

    if (prevRect.expanded (2.0f).contains (p)) { if (onStep) onStep (-1); return; }
    if (nextRect.expanded (2.0f).contains (p)) { if (onStep) onStep (1);  return; }

    if (boxRect.contains (p))
    {
        const auto& list = Presets::all();
        juce::PopupMenu menu;
        const int current = getPresetIndex ? getPresetIndex() : 0;

        for (int i = 0; i < (int) list.size(); ++i)
            menu.addItem (i + 1, list[(size_t) i].name, true, i == current);

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this)
                                                     .withTargetScreenArea (localAreaToGlobal (boxRect.toNearestInt())),
                            [this] (int result)
                            {
                                if (result > 0 && onPresetChosen)
                                    onPresetChosen (result - 1);
                            });
    }
}

} // namespace ui
