#include "Theme.h"
#include "FontData.h"

namespace theme
{

//==============================================================================
namespace
{
    juce::Typeface::Ptr michromaFace, shareTechFace, chakraFace, chakraBoldFace;
    bool fontsReady = false;

    juce::Typeface::Ptr decode (const char* b64)
    {
        if (b64 == nullptr)
            return nullptr;

        juce::MemoryOutputStream decoded;
        if (! juce::Base64::convertFromBase64 (decoded, juce::String (b64)))
            return nullptr;

        return juce::Typeface::createSystemTypefaceFor (decoded.getData(), decoded.getDataSize());
    }

    juce::Typeface::Ptr pick (Face face)
    {
        initialiseFonts();  // safe: only ever called from the message thread

        juce::Typeface::Ptr tf;
        switch (face)
        {
            case Face::logo:
            case Face::label:       tf = michromaFace; break;
            case Face::mono:        tf = shareTechFace; break;
            case Face::caption:     tf = chakraFace; break;
            case Face::captionBold: tf = chakraBoldFace; break;
            case Face::value:       tf = chakraBoldFace; break;
        }

        if (tf == nullptr)
            tf = juce::Typeface::createSystemTypefaceFor (juce::Font (juce::FontOptions (12.0f)));

        return tf;
    }
}

void initialiseFonts()
{
    if (fontsReady)
        return;

    fontsReady = true;
    michromaFace   = decode (GridVoxFonts::michromaB64);
    shareTechFace  = decode (GridVoxFonts::shareTechMonoBase64);
    chakraFace     = decode (GridVoxFonts::chakraRegularBase64);
    chakraBoldFace = decode (GridVoxFonts::chakraSemiBoldBase64);
}

juce::Font font (Face face, float heightPx)
{
    return juce::Font (juce::FontOptions (pick (face)).withHeight (heightPx));
}

juce::Font fontFitted (Face face, float heightPx, const juce::String& text, float targetWidth)
{
    auto base = font (face, heightPx);

    if (text.isEmpty() || targetWidth <= 1.0f)
        return base;

    const float natural = base.getStringWidthFloat (text);
    if (natural <= 0.1f)
        return base;

    auto widthWithKerning = [] (const juce::Font& f, const juce::String& t, float k)
    {
        const auto gaps = (float) juce::jmax (0, t.length() - 1);
        return f.getStringWidthFloat (t) + k * f.getHeight() * gaps;
    };

    // Tracking is only ever a fine adjustment: cap it so labels never spread out
    // into letter-soup, and shrink the glyphs when the string genuinely does not fit.
    auto kerningFor = [&] (const juce::Font& f, float naturalWidth, float height)
    {
        const auto perChar = juce::jmax (1.0f, height * (float) text.length());
        return juce::jlimit (-0.06f, 0.16f, (targetWidth - naturalWidth) / perChar);
    };

    auto k = kerningFor (base, natural, heightPx);

    if (widthWithKerning (base, text, k) > targetWidth)
    {
        const auto scaled = juce::jmax (4.0f, heightPx * targetWidth / widthWithKerning (base, text, k));
        base = font (face, scaled);
        k = kerningFor (base, base.getStringWidthFloat (text), scaled);
    }

    return base.withExtraKerningFactor (k);
}

//==============================================================================
juce::Path chamferPath (juce::Rectangle<float> b, Corners c)
{
    juce::Path p;
    p.startNewSubPath (b.getX() + c.tl, b.getY());
    p.lineTo (b.getRight() - c.tr, b.getY());
    if (c.tr > 0.0f) p.lineTo (b.getRight(), b.getY() + c.tr);
    p.lineTo (b.getRight(), b.getBottom() - c.br);
    if (c.br > 0.0f) p.lineTo (b.getRight() - c.br, b.getBottom());
    p.lineTo (b.getX() + c.bl, b.getBottom());
    if (c.bl > 0.0f) p.lineTo (b.getX(), b.getBottom() - c.bl);
    p.lineTo (b.getX(), b.getY() + c.tl);
    if (c.tl > 0.0f) p.closeSubPath();
    else p.closeSubPath();
    return p;
}

void drawPanel (juce::Graphics& g, juce::Rectangle<float> bounds, Corners c, float fillTopAlpha, bool drawFill)
{
    const auto path = chamferPath (bounds, c);

    if (drawFill)
    {
        juce::ColourGradient grad (panelTop.withMultipliedAlpha (fillTopAlpha), bounds.getCentreX(), bounds.getY(),
                                   panelBottom, bounds.getCentreX(), bounds.getBottom(), false);
        grad.addColour (0.55, panelDeep);
        g.setGradientFill (grad);
        g.fillPath (path);

        // Inner top highlight, keeps the modules feeling like machined metal.
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (path);
        g.setColour (cyan.withAlpha (0.045f));
        g.fillRect (bounds.getX(), bounds.getY(), bounds.getWidth(), 1.2f);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRect (bounds.getX(), bounds.getBottom() - 1.6f, bounds.getWidth(), 1.6f);
    }

    // Outer glow then the crisp 1 px line.
    g.setColour (border.withAlpha (0.14f));
    g.strokePath (path, juce::PathStrokeType (2.4f));
    g.setColour (border);
    g.strokePath (path, juce::PathStrokeType (1.0f));

    // The chamfered diagonals catch the light in the original artwork.
    if (c.tl > 0.0f || c.tr > 0.0f || c.br > 0.0f || c.bl > 0.0f)
    {
        const auto& b = bounds;
        juce::Path edges;
        auto addDiagonal = [&edges] (float x0, float y0, float x1, float y1)
        {
            edges.startNewSubPath (x0, y0);
            edges.lineTo (x1, y1);
        };
        if (c.tl > 0.0f) addDiagonal (b.getX(), b.getY() + c.tl, b.getX() + c.tl, b.getY());
        if (c.tr > 0.0f) addDiagonal (b.getRight() - c.tr, b.getY(), b.getRight(), b.getY() + c.tr);
        if (c.br > 0.0f) addDiagonal (b.getRight(), b.getBottom() - c.br, b.getRight() - c.br, b.getBottom());
        if (c.bl > 0.0f) addDiagonal (b.getX() + c.bl, b.getBottom(), b.getX(), b.getBottom() - c.bl);

        g.setColour (borderBright.withAlpha (0.30f));
        g.strokePath (edges, juce::PathStrokeType (2.2f));
        g.setColour (borderBright);
        g.strokePath (edges, juce::PathStrokeType (1.0f));
    }
}

void drawLabelledPanel (juce::Graphics& g, juce::Rectangle<float> bounds, Corners c,
                        const juce::String& label, float labelX, float labelHeight, juce::Colour labelColour)
{
    drawPanel (g, bounds, c);

    if (label.isEmpty())
        return;

    auto f = font (Face::label, labelHeight);
    const float tracked = f.getStringWidthFloat (label) + labelHeight * 1.06f * (float) label.length();
    const float textWidth = tracked + 4.0f;

    // Punch a hole in the top border so the text sits "in" the frame.
    g.setColour (panelDeep);
    g.fillRect (labelX - 3.0f, bounds.getY() - 0.8f, textWidth + 6.0f, 1.8f);
    g.setColour (panelTop.brighter (0.05f).withAlpha (0.0f));
    g.fillRect (labelX - 3.0f, bounds.getY() - 1.6f, textWidth + 6.0f, 1.4f);

    const juce::Rectangle<float> labelArea (labelX, bounds.getY() - labelHeight * 0.66f, textWidth, labelHeight * 1.7f);
    auto tracker = f.withExtraKerningFactor (0.16f);

    // A 0.4 px second pass thickens the very small glyphs so they stay legible.
    g.setFont (tracker);
    g.setColour (labelColour.withAlpha (0.55f));
    g.drawText (label, labelArea.translated (0.4f, 0.0f), juce::Justification::centredLeft, false);
    g.drawText (label, labelArea.translated (0.0f, 0.4f), juce::Justification::centredLeft, false);
    g.setColour (labelColour);
    g.drawText (label, labelArea, juce::Justification::centredLeft, false);
}

void drawText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area,
               const juce::Font& f, juce::Colour colour, juce::Justification just, float tracking)
{
    auto used = f;
    if (tracking > 0.0f)
        used = f.withExtraKerningFactor (tracking);

    g.setFont (used);
    g.setColour (colour);
    g.drawText (text, area, just, false);
}

void drawLed (juce::Graphics& g, juce::Point<float> centre, float radius, juce::Colour colour, bool lit)
{
    const auto glow = lit ? colour.withAlpha (0.30f) : juce::Colours::transparentBlack;
    if (lit)
    {
        g.setGradientFill (juce::ColourGradient (glow, centre.x, centre.y,
                                                 juce::Colours::transparentBlack, centre.x + radius * 3.2f, centre.y, true));
        g.fillEllipse (centre.x - radius * 3.2f, centre.y - radius * 3.2f, radius * 6.4f, radius * 6.4f);
        g.setColour (colour);
    }
    else
    {
        g.setColour (colour.withAlpha (0.22f));
    }

    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    if (lit)
    {
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.fillEllipse (centre.x - radius * 0.42f, centre.y - radius * 0.55f, radius * 0.75f, radius * 0.75f);
    }
}

//==============================================================================
void drawKnob (juce::Graphics& g, juce::Rectangle<float> bounds, float t, bool bipolar, float arcThickness)
{
    const auto centre = bounds.getCentre();
    const float tickR = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    // The artwork keeps an absolute ~4 px gap between the arc and the tick ring,
    // so small knobs read as proportionally tighter.
    const float arcR  = juce::jmax (5.0f, tickR - 4.2f);
    const float faceR = arcR * 0.78f;
    const float scale = tickR / 26.0f;          // 1.0 for the 52 px "large" knob

    const float startDeg = -135.0f;
    const float sweepDeg = 270.0f;

    auto pointAt = [centre] (float radius, float deg)
    {
        const float rad = juce::degreesToRadians (deg);
        return juce::Point<float> (centre.x + radius * std::sin (rad),
                                   centre.y - radius * std::cos (rad));
    };

    // --- tick ring -------------------------------------------------------
    g.setColour (juce::Colour (0xff1b5866).withAlpha (0.75f));
    const int numTicks = 20;
    for (int i = 0; i <= numTicks; ++i)
    {
        const float deg = startDeg + sweepDeg * (float) i / (float) numTicks;
        const auto inner = pointAt (tickR - juce::jmax (1.2f, 1.7f * scale), deg);
        const auto outer = pointAt (tickR - 0.2f, deg);
        g.drawLine ({ inner, outer }, juce::jmax (0.6f, 0.75f * scale));
    }

    // --- recessed face ---------------------------------------------------
    {
        juce::ColourGradient face (juce::Colour (0xff0a1c23), centre.x, centre.y - faceR,
                                   juce::Colour (0xff020507), centre.x, centre.y + faceR, true);
        face.addColour (0.45, juce::Colour (0xff061218));
        face.addColour (0.85, juce::Colour (0xff03080b));
        g.setGradientFill (face);
        g.fillEllipse (centre.x - faceR, centre.y - faceR, faceR * 2.0f, faceR * 2.0f);

        g.setColour (juce::Colour (0xff1a4450).withAlpha (0.75f));
        g.drawEllipse (centre.x - faceR, centre.y - faceR, faceR * 2.0f, faceR * 2.0f,
                       juce::jmax (0.7f, 0.9f * scale));
    }

    // --- track arc -------------------------------------------------------
    {
        juce::Path track;
        track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f,
                             juce::degreesToRadians (startDeg),
                             juce::degreesToRadians (startDeg + sweepDeg), true);
        g.setColour (juce::Colour (0xff123c48).withAlpha (0.55f));
        g.strokePath (track, juce::PathStrokeType (juce::jmax (1.0f, 1.3f * scale),
                                                   juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // --- value arc -------------------------------------------------------
    const float fromDeg = bipolar ? 0.0f : startDeg;
    const float valueDeg = bipolar ? (t - 0.5f) * sweepDeg : startDeg + t * sweepDeg;

    if (std::abs (valueDeg - fromDeg) > 0.6f)
    {
        juce::Path value;
        const float a0 = juce::degreesToRadians (juce::jmin (fromDeg, valueDeg));
        const float a1 = juce::degreesToRadians (juce::jmax (fromDeg, valueDeg));
        // addCentredArc takes absolute start/finish angles (radians), 0 = 12 o'clock.
        value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, a0, a1, true);

        const float thickness = juce::jmax (1.4f, 2.0f * scale);

        g.setColour (cyan.withAlpha (0.07f));
        g.strokePath (value, juce::PathStrokeType (thickness * 3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (cyan.withAlpha (0.22f));
        g.strokePath (value, juce::PathStrokeType (thickness * 1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (cyan);
        g.strokePath (value, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (cyanHot.withAlpha (0.55f));
        g.strokePath (value, juce::PathStrokeType (thickness * 0.45f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // --- pointer ---------------------------------------------------------
    {
        const auto inner = pointAt (faceR * 0.16f, valueDeg);
        const auto outer = pointAt (faceR * 0.78f, valueDeg);
        const float thickness = juce::jmax (1.1f, 1.6f * scale);

        g.setColour (cyan.withAlpha (0.22f));
        g.drawLine ({ inner, outer }, thickness * 3.0f);
        g.setColour (cyan);
        g.drawLine ({ inner, outer }, thickness);
        g.setColour (cyanHot);
        g.drawLine ({ inner, outer }, thickness * 0.5f);
    }

    // centre pin
    {
        const float r = juce::jmax (0.55f, 0.8f * scale);
        g.setColour (juce::Colour (0xff2d6d7c));
        g.fillEllipse (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f);
    }
}

//==============================================================================
void drawButtonFace (juce::Graphics& g, juce::Rectangle<float> bounds, bool selected, bool hovered)
{
    const auto b = bounds.reduced (0.6f);

    const auto cut = juce::jmin (3.0f, b.getHeight() * 0.30f);
    const auto path = chamferPath (b, Corners::small (cut));

    if (selected)
    {
        juce::ColourGradient grad (juce::Colour (0xff1e6a79), b.getCentreX(), b.getY(),
                                   juce::Colour (0xff0b3a46), b.getCentreX(), b.getBottom(), false);
        grad.addColour (0.45, juce::Colour (0xff175666));
        g.setGradientFill (grad);
        g.fillPath (path);

        // Wide soft rim plus the crisp 1 px outline - the artwork's selected
        // buttons glow around their whole perimeter.
        g.setColour (juce::Colour (0xff3fc6dc).withAlpha (0.22f));
        g.strokePath (path, juce::PathStrokeType (2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colour (0xff54dcf2).withAlpha (0.9f));
        g.strokePath (path, juce::PathStrokeType (0.9f));
    }
    else
    {
        juce::ColourGradient grad (juce::Colour (0xff08171e), b.getCentreX(), b.getY(),
                                   juce::Colour (0xff04101a), b.getCentreX(), b.getBottom(), false);
        g.setGradientFill (grad);
        g.fillPath (path);

        g.setColour ((hovered ? borderBright : juce::Colour (0xff16414e)).withAlpha (0.95f));
        g.strokePath (path, juce::PathStrokeType (0.9f));
    }
}

void drawMeterTrack (juce::Graphics& g, juce::Rectangle<float> bounds, int numSegments)
{
    const float gap = 2.0f;
    const float segH = (bounds.getHeight() - gap * (float) (numSegments - 1)) / (float) numSegments;

    for (int i = 0; i < numSegments; ++i)
    {
        const float y = bounds.getBottom() - (float) (i + 1) * segH - (float) i * gap;
        g.setColour (juce::Colour (0xff0b2129));
        g.fillRect (bounds.getX(), y, bounds.getWidth(), segH);
    }
}

} // namespace theme
