#pragma once

/*  Plain-old-data snapshot of every parameter that the audio thread needs.
    The processor fills one of these once per block from the APVTS, so the DSP
    code never touches the value tree, never blocks and never allocates.
*/
namespace gv
{

struct ChainParameters
{
    float gate        = 20.0f;
    float lowCutHz    = 80.0f;
    int   lowCutSlope = 36;      // 12 / 24 / 36 dB per octave

    float mud    = 25.0f;
    float grace  = 35.0f;
    float deEss  = 30.0f;
    float warmth = 20.0f;

    float fast   = 15.0f;
    float smooth = 20.0f;

    float eqGain[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float eqFreq[4] = { 120.0f, 300.0f, 2500.0f, 12000.0f };

    float reverbMix    = 45.0f;
    int   reverbType   = 1;      // 0 room, 1 plate, 2 hall
    float reverbDecay  = 0.46f;  // seconds (RT60), derived from host tempo + division
    float reverbLoCut  = 320.0f;
    float reverbHiCut  = 8000.0f;

    float outputDb = 4.5f;
    bool  bypass   = false;
};

/** Real-time status values pushed from the DSP to the editor. */
struct ChainStatus
{
    float inputPeak  = 0.0f;   // linear, pre-processing
    float outputPeak = 0.0f;   // linear, post limiter

    float gateReductionDb = 0.0f;
    float mudReductionDb  = 0.0f;
    float deEssReduction  = 0.0f;
    float fastReductionDb = 0.0f;
    float smoothReductionDb = 0.0f;
    float limiterReductionDb = 0.0f;

    bool gateActive    = false;
    bool mudActive     = false;
    bool deEssActive   = false;
    bool limiterActive = false;
};

} // namespace gv
