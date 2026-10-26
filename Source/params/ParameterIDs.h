#pragma once

/*  Central list of parameter identifiers.
    Keeping the strings in one place avoids typos between the processor, the
    presets, the editor and the DSP snapshot code.
*/
namespace ParamIDs
{
    // ---- Clean-up -----------------------------------------------------------
    static constexpr const char* gate        = "gate";         // 0..100 %
    static constexpr const char* lowCutFreq  = "lowcut_freq";  // 20..400 Hz
    static constexpr const char* lowCutSlope = "lowcut_slope"; // 12 / 24 / 36 dB per oct

    // ---- Character ----------------------------------------------------------
    static constexpr const char* mud         = "mud";          // 0..100 %
    static constexpr const char* grace       = "grace";        // 0..100 %
    static constexpr const char* deEss       = "deess";        // 0..100 %
    static constexpr const char* warmth      = "warmth";       // 0..100 %

    // ---- Dynamics -----------------------------------------------------------
    static constexpr const char* fast        = "fast";         // 0..100 %
    static constexpr const char* smooth      = "smooth";       // 0..100 %

    // ---- Tone EQ ------------------------------------------------------------
    static constexpr const char* eqGain[4] = { "eq1_gain", "eq2_gain", "eq3_gain", "eq4_gain" };
    static constexpr const char* eqFreq[4] = { "eq1_freq", "eq2_freq", "eq3_freq", "eq4_freq" };

    // ---- Space --------------------------------------------------------------
    static constexpr const char* reverbMix   = "reverb";       // 0..100 %
    static constexpr const char* reverbType  = "reverb_type";  // Room / Plate / Hall
    static constexpr const char* reverbTime  = "reverb_time";  // note division (tempo synced)
    static constexpr const char* reverbLoCut = "reverb_locut"; // Hz
    static constexpr const char* reverbHiCut = "reverb_hicut"; // Hz

    // ---- Output -------------------------------------------------------------
    static constexpr const char* output      = "output";       // -24..+24 dB
    static constexpr const char* bypass      = "bypass";       // bool
}
