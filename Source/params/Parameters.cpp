#include "Parameters.h"
#include "ParameterIDs.h"

using namespace juce;

namespace Params
{
    static const TimeDivision divisions[] = {
        { "1/32", 0.125 }, { "1/16", 0.25 }, { "1/8", 0.5 },
        { "1/4", 1.0 },    { "1/2", 2.0 },   { "1/1", 4.0 }
    };

    const TimeDivision& timeDivision (int index) { return divisions[jlimit (0, numTimeDivisions() - 1, index)]; }
    int numTimeDivisions() { return (int) (sizeof (divisions) / sizeof (divisions[0])); }

    const char* eqBandNames[numEqBands]     = { "LOWS", "LO-MID", "HI-MID", "HIGHS" };
    const char* eqBandCharacter[numEqBands] = { "TIGHT LOW", "GROWL MID", "FOCUS MID", "SILK AIR" };

    double reverbDecaySeconds (int divisionIndex, double bpm)
    {
        const auto d = timeDivision (divisionIndex);
        const auto beatsPerSecond = jmax (20.0, bpm) / 60.0;
        // A note value reads as an RT60. Doubling keeps the shorter divisions musically
        // useful while remaining exactly proportional to the host tempo.
        return 2.0 * d.beats / beatsPerSecond;
    }

    //==============================================================================
    namespace Text
    {
        String percent (float v) { return String (roundToInt (v)) + "%"; }

        String hz (float v)
        {
            if (v >= 1000.0f)
            {
                const auto k = v / 1000.0f;
                return (k < 10.0f ? String (k, 2) : String (k, 1)) + " KHZ";
            }
            return String (roundToInt (v)) + " HZ";
        }

        String db (float v, bool alwaysSign)
        {
            const auto txt = String (std::abs (v), 1) + " DB";
            if (v < -0.049f) return "-" + txt;
            if (v > 0.049f || alwaysSign) return "+" + txt;
            return txt;
        }

        String slope (int index) { return String (12 * (index + 1)); }
    }

    //==============================================================================
    static String noteName (int i) { return timeDivision (i).text; }

    // skew factor that places 80 Hz at 53 % of the travel (matches the artwork)
    static constexpr float lowCutSkew = 0.471f;

    AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        AudioProcessorValueTreeState::ParameterLayout layout;

        auto floatParam = [&] (const char* id, const String& name, NormalisableRange<float> range,
                               float def, const String& unit = {},
                               std::function<String (float, int)> toText = {},
                               std::function<float (const String&)> fromText = {})
        {
            AudioParameterFloatAttributes attrs;
            if (unit.isNotEmpty()) attrs = attrs.withLabel (unit);
            if (toText)   attrs = attrs.withStringFromValueFunction (std::move (toText));
            if (fromText) attrs = attrs.withValueFromStringFunction (std::move (fromText));
            layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, range, def, attrs));
        };

        auto percentRange = NormalisableRange<float> (0.0f, 100.0f, 0.1f);

        // Clean-up ---------------------------------------------------------------
        floatParam (ParamIDs::gate, "Gate", percentRange, 20.0f, "%",
                    [] (float v, int) { return Text::percent (v); });
        floatParam (ParamIDs::lowCutFreq, "Low Cut Freq",
                    NormalisableRange<float> (20.0f, 250.0f, 0.0f, lowCutSkew), 80.0f, "Hz",
                    [] (float v, int) { return Text::hz (v); });
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::lowCutSlope, 1 },
                                                            "Low Cut Slope", StringArray { "12", "24", "36" }, 2));

        // Character --------------------------------------------------------------
        floatParam (ParamIDs::mud, "Mud", percentRange, 25.0f, "%",
                    [] (float v, int) { return Text::percent (v); });
        floatParam (ParamIDs::grace, "Grace", percentRange, 35.0f, "%",
                    [] (float v, int) { return Text::percent (v); });
        floatParam (ParamIDs::deEss, "De-Ess", percentRange, 30.0f, "%",
                    [] (float v, int) { return Text::percent (v); });
        floatParam (ParamIDs::warmth, "Warmth", percentRange, 20.0f, "%",
                    [] (float v, int) { return Text::percent (v); });

        // Dynamics ---------------------------------------------------------------
        floatParam (ParamIDs::fast, "Fast", percentRange, 15.0f, "%",
                    [] (float v, int) { return Text::percent (v); });
        floatParam (ParamIDs::smooth, "Smooth", percentRange, 20.0f, "%",
                    [] (float v, int) { return Text::percent (v); });

        // Tone EQ ----------------------------------------------------------------
        // The skew factors put each default exactly where the artwork draws it.
        const struct { float gMin, gMax; float fMin, fMax, fDef, fSkew; } bandDefaults[numEqBands] =
        {
            { -12.0f, 12.0f,  40.0f,  320.0f,  120.0f, 0.685f },
            { -12.0f, 12.0f, 120.0f,  900.0f,  300.0f, 0.565f },
            { -12.0f, 12.0f, 800.0f, 4000.0f, 2500.0f, 1.125f },
            { -12.0f, 12.0f, 4000.0f, 16000.0f, 12000.0f, 0.422f }
        };

        for (int i = 0; i < numEqBands; ++i)
        {
            const auto& d = bandDefaults[i];
            const auto bandName = String (eqBandNames[i]);

            floatParam (ParamIDs::eqGain[i], bandName + " Gain",
                        NormalisableRange<float> (d.gMin, d.gMax, 0.01f), 0.0f, "dB",
                        [] (float v, int) { return Text::db (v, false); });

            floatParam (ParamIDs::eqFreq[i], bandName + " Freq",
                        NormalisableRange<float> (d.fMin, d.fMax, 0.0f, d.fSkew), d.fDef, "Hz",
                        [] (float v, int) { return Text::hz (v); });
        }

        // Space ------------------------------------------------------------------
        floatParam (ParamIDs::reverbMix, "Reverb", percentRange, 45.0f, "%",
                    [] (float v, int) { return Text::percent (v); });
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::reverbType, 1 },
                                                            "Space Type", StringArray { "ROOM", "PLATE", "HALL" }, 1));
        {
            StringArray names;
            for (int i = 0; i < numTimeDivisions(); ++i)
                names.add (noteName (i));

            layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::reverbTime, 1 },
                                                                "Space Time", names, 2));
        }
        floatParam (ParamIDs::reverbLoCut, "Space Lo Cut",
                    NormalisableRange<float> (100.0f, 1000.0f, 0.0f, 0.373f), 320.0f, "Hz",
                    [] (float v, int) { return Text::hz (v); });
        floatParam (ParamIDs::reverbHiCut, "Space Hi Cut",
                    NormalisableRange<float> (2000.0f, 16000.0f, 0.0f, 0.454f), 8000.0f, "Hz",
                    [] (float v, int) { return Text::hz (v); });

        // Output -----------------------------------------------------------------
        floatParam (ParamIDs::output, "Output", NormalisableRange<float> (-8.0f, 8.0f, 0.01f), 4.5f, "dB",
                    [] (float v, int) { return Text::db (v, true); });

        layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::bypass, 1 }, "Bypass", false));

        return layout;
    }
}
