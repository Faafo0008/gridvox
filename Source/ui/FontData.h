#pragma once
// Bundled open-source typefaces (SIL Open Font License - see README).
// Stored base64-encoded to keep translation units small and compile times low.
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>

namespace GridVoxFonts
{
    extern const char* michromaB64;
    extern const char* shareTechMonoBase64;
    extern const char* chakraRegularBase64;
    extern const char* chakraSemiBoldBase64;

    /** Decodes one of the bundled faces. Thread-safe-once (call from the message thread). */
    juce::Typeface::Ptr loadTypeface (const char* base64Data);
}
