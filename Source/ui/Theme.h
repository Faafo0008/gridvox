#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

/*  Central design system: colours, type faces, and the vector drawing helpers
    used by every widget. Everything is expressed in the reference artwork's
    coordinate space (560 x 499 px).
*/
namespace theme
{
    //== Colours (sampled from the reference artwork) ==========================
    const juce::Colour windowBg      { 0xff020507 };
    const juce::Colour titleBarBg    { 0xff252525 };
    const juce::Colour titleBarHi    { 0xff2e2e2e };
    const juce::Colour panelTop      { 0xff0a1a22 };
    const juce::Colour panelBottom   { 0xff070e15 };
    const juce::Colour panelDeep     { 0xff050b10 };
    const juce::Colour border        { 0xff1d6c7e };
    const juce::Colour borderBright  { 0xff3ec9de };
    const juce::Colour cyan          { 0xff64dff0 };
    const juce::Colour cyanHot       { 0xffa9f6ff };
    const juce::Colour cyanDeep      { 0xff0f6d80 };
    const juce::Colour cyanDim       { 0xff14424f };
    const juce::Colour titleText     { 0xfff2fdff };
    const juce::Colour valueText     { 0xff549aa9 };
    const juce::Colour captionText   { 0xff2f6473 };
    const juce::Colour mutedText     { 0xff23525e };
    const juce::Colour orange        { 0xffff7a24 };
    const juce::Colour green         { 0xff43d9a0 };
    const juce::Colour greyText      { 0xff97a3a8 };

    //== Dimensions ===========================================================
    namespace dim
    {
        constexpr float width  = 560.0f;
        constexpr float height = 499.0f;
        constexpr float titleH = 18.0f;
        constexpr float chamfer = 10.0f;
    }

    //== Type faces ===========================================================
    enum class Face { logo, label, mono, caption, captionBold, value };

    /** Loads the bundled typefaces. Call once from the message thread. */
    void initialiseFonts();

    juce::Font font (Face face, float heightPx);

    /** Returns a copy of the font whose string width matches targetWidth. */
    juce::Font fontFitted (Face face, float heightPx, const juce::String& text, float targetWidth);

    //== Drawing helpers ======================================================
    struct Corners
    {
        float tl = 0.0f, tr = 0.0f, br = 0.0f, bl = 0.0f;
        static Corners modules()  { return { dim::chamfer, 0.0f, dim::chamfer, 0.0f }; }
        static Corners group()    { return { dim::chamfer, dim::chamfer, dim::chamfer, dim::chamfer }; }
        static Corners small(float c) { return { c, c, c, c }; }
    };

    juce::Path chamferPath (juce::Rectangle<float> bounds, Corners c);

    /** Draws the signature neon frame: dark gradient fill + glowing 1px border. */
    void drawPanel (juce::Graphics&, juce::Rectangle<float> bounds, Corners c,
                    float fillTopAlpha = 1.0f, bool drawFill = true);

    /** Panel with a title sitting on its top edge (the text interrupts the border). */
    void drawLabelledPanel (juce::Graphics&, juce::Rectangle<float> bounds, Corners c,
                            const juce::String& label, float labelX, float labelHeight,
                            juce::Colour labelColour);

    void drawText (juce::Graphics&, const juce::String& text, juce::Rectangle<float> area,
                   const juce::Font& font, juce::Colour colour, juce::Justification just = juce::Justification::centredLeft,
                   float tracking = 0.0f);

    /** Small neon dot with a glow, used for the module status LEDs. */
    void drawLed (juce::Graphics&, juce::Point<float> centre, float radius, juce::Colour colour, bool lit);

    /** Draws a rotary control of the given diameter, t = 0..1, bipolar draws from 12 o'clock. */
    void drawKnob (juce::Graphics&, juce::Rectangle<float> bounds, float t, bool bipolar, float arcThickness = 2.4f);

    /** Chamfered button face. */
    void drawButtonFace (juce::Graphics&, juce::Rectangle<float> bounds, bool selected, bool hovered);

    /** Vertical segmented meter. */
    void drawMeterTrack (juce::Graphics&, juce::Rectangle<float> bounds, int numSegments);

    //== Layout (reference pixels) ===========================================
    namespace layout
    {
        inline juce::Rectangle<float> rect (float x, float y, float w, float h) { return { x, y, w, h }; }

        // Meters
        inline const juce::Rectangle<float> inMeter  { 12.0f,  63.0f, 31.0f, 407.0f };
        inline const juce::Rectangle<float> outMeter { 520.0f, 63.0f, 31.0f, 407.0f };

        // Row A
        inline const juce::Rectangle<float> gate     { 48.0f,  63.0f, 60.0f, 131.0f };
        inline const juce::Rectangle<float> lowCut   { 115.0f, 63.0f, 59.0f, 131.0f };
        inline const juce::Rectangle<float> mud      { 181.0f, 63.0f, 59.0f, 131.0f };
        inline const juce::Rectangle<float> grace    { 248.0f, 63.0f, 59.0f, 131.0f };
        inline const juce::Rectangle<float> dynamics { 314.0f, 63.0f, 200.0f, 131.0f };

        // Tone EQ
        inline const juce::Rectangle<float> toneEq   { 48.0f, 200.0f, 466.0f, 131.0f };

        // Row C
        inline const juce::Rectangle<float> deEss    { 48.0f, 337.0f, 72.0f, 131.0f };
        inline const juce::Rectangle<float> warmth   { 126.0f, 337.0f, 72.0f, 131.0f };
        inline const juce::Rectangle<float> space    { 203.0f, 337.0f, 233.0f, 131.0f };
        inline const juce::Rectangle<float> output   { 442.0f, 337.0f, 72.0f, 131.0f };

        // Header widgets
        inline const juce::Rectangle<float> prevButton { 274.0f, 32.0f, 15.0f, 17.0f };
        inline const juce::Rectangle<float> presetBox  { 295.0f, 32.0f, 137.0f, 17.0f };
        inline const juce::Rectangle<float> nextButton { 439.0f, 32.0f, 15.0f, 17.0f };
        inline const juce::Rectangle<float> bypassButton { 473.0f, 32.0f, 67.0f, 17.0f };

        // Footer
        inline const juce::Rectangle<float> latencyBadge { 13.0f, 476.0f, 92.0f, 14.0f };
        inline const juce::Rectangle<float> licensedBadge { 437.0f, 476.0f, 110.0f, 14.0f };
        inline const juce::Rectangle<float> footerHint { 150.0f, 476.0f, 265.0f, 14.0f };
    }
}
