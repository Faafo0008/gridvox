#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace Params
{
    /** Note divisions used by the tempo-synced reverb decay control. */
    struct TimeDivision { const char* text; double beats; };
    const TimeDivision& timeDivision (int index);
    int numTimeDivisions();

    static constexpr int numEqBands = 4;
    extern const char* eqBandNames[numEqBands];      // LOWS / LO-MID / HI-MID / HIGHS
    extern const char* eqBandCharacter[numEqBands];  // TIGHT LOW / GROWL MID / FOCUS MID / SILK AIR

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    /** The reverb decay in seconds for the given division index and tempo. */
    double reverbDecaySeconds (int divisionIndex, double bpm);

    //==============================================================================
    /** Value text formatting that matches the reference artwork (upper-case units). */
    namespace Text
    {
        juce::String percent (float v);                 // "20%"
        juce::String hz (float v);                      // "80 HZ" / "2.50 KHZ" / "12.0 KHZ"
        juce::String db (float v, bool alwaysSign);     // "0.0 DB" / "+4.5 DB"
        juce::String slope (int index);                 // "12" / "24" / "36"
    }
}
