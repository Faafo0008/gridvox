#pragma once

#include "ChainParameters.h"
#include "CleanupStage.h"
#include "EqStage.h"
#include "DeEsserStage.h"
#include "DynamicsStage.h"
#include "WarmthStage.h"
#include "ReverbStage.h"
#include "OutputStage.h"

/*  The complete signal chain, in the same order as the artwork:

        input -> low cut -> gate -> mud -> grace -> tone EQ -> de-esser
              -> warmth -> FAST -> SMOOTH -> space -> output gain/limiter -> out

    Everything is stereo, sample-rate agnostic and allocation free after
    prepare() has been called.
*/
namespace gv
{

class VocalChain
{
public:
    void prepare (double sampleRate, int /*maximumBlockSize*/)
    {
        fs = sampleRate;

        lowCut.prepare (fs);
        gate.prepare (fs);
        mud.prepare (fs);
        grace.prepare (fs);
        eq.prepare (fs);
        deEsser.prepare (fs);
        warmth.prepare (fs);
        fast.prepare (fs, { -18.0f, 6.0f, 3.0, 60.0, 6.0f });
        smooth.prepare (fs, { -26.0f, 2.5f, 25.0, 350.0, 8.0f });
        reverb.prepare (fs);
        output.prepare (fs);

        status = ChainStatus{};
        bypassed = false;
        prepared = true;
    }

    void reset()
    {
        lowCut.reset();
        gate.reset();
        mud.reset();
        grace.reset();
        eq.reset();
        deEsser.reset();
        warmth.reset();
        fast.reset();
        smooth.reset();
        reverb.reset();
        output.reset();
    }

    /** Called once per block (never per sample). */
    void setParameters (const ChainParameters& p)
    {
        lowCut.setTarget (p.lowCutHz, p.lowCutSlope);
        gate.setAmount (p.gate);
        mud.setAmount (p.mud);
        grace.setAmount (p.grace);
        eq.setParams (p.eqGain, p.eqFreq, firstParameterPass);
        deEsser.setAmount (p.deEss);
        warmth.setAmount (p.warmth);
        fast.setAmount (p.fast);
        smooth.setAmount (p.smooth);
        reverb.setParams (p.reverbMix, p.reverbType, p.reverbDecay, p.reverbLoCut, p.reverbHiCut);
        output.setGainDb (p.outputDb, firstParameterPass);
        bypassed = p.bypass;
        firstParameterPass = false;
    }

    void process (float* left, float* right, int numSamples)
    {
        if (! prepared || numSamples <= 0)
            return;

        // Input meter is measured pre everything.
        float inPeak = 0.0f;

        if (bypassed)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                const float l = left[i], r = right[i];
                if (! std::isfinite (l) || ! std::isfinite (r)) { left[i] = 0.0f; right[i] = 0.0f; continue; }
                inPeak = std::max (inPeak, std::max (std::abs (l), std::abs (r)));
            }
            status.inputPeak = inPeak;
            status.outputPeak = inPeak;
            status.gateActive = status.mudActive = status.deEssActive = status.limiterActive = false;
            status.gateReductionDb = status.mudReductionDb = status.deEssReduction = 0.0f;
            status.fastReductionDb = status.smoothReductionDb = status.limiterReductionDb = 0.0f;
            return;
        }

        for (int i = 0; i < numSamples; ++i)
            inPeak = std::max (inPeak, std::max (std::abs (left[i]), std::abs (right[i])));

        lowCut.process (left, right, numSamples);
        gate.process (left, right, numSamples);
        mud.process (left, right, numSamples);
        grace.process (left, right, numSamples);
        eq.process (left, right, numSamples);
        deEsser.process (left, right, numSamples);
        warmth.process (left, right, numSamples);
        fast.process (left, right, numSamples);
        smooth.process (left, right, numSamples);
        reverb.process (left, right, numSamples);
        output.process (left, right, numSamples);

        float outPeak = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            outPeak = std::max (outPeak, std::max (std::abs (left[i]), std::abs (right[i])));

        status.inputPeak  = inPeak;
        status.outputPeak = outPeak;
        status.gateReductionDb   = gate.getReductionDb();
        status.mudReductionDb    = mud.getReductionDb();
        status.deEssReduction    = deEsser.getReductionDb();
        status.fastReductionDb   = fast.getReductionDb();
        status.smoothReductionDb = smooth.getReductionDb();
        status.limiterReductionDb = output.getReductionDb();
        status.gateActive    = gate.isActive();
        status.mudActive     = mud.isActive();
        status.deEssActive   = deEsser.isActive();
        status.limiterActive = output.isActive();
    }

    const ChainStatus& getStatus() const noexcept { return status; }
    static constexpr int getLatencySamples() noexcept { return WarmthStage::latencySamples; }

private:
    double fs = 44100.0;
    bool prepared = false, bypassed = false, firstParameterPass = true;

    LowCutStage      lowCut;
    GateStage        gate;
    MudStage         mud;
    GraceStage       grace;
    EqStage          eq;
    DeEsserStage     deEsser;
    WarmthStage      warmth;
    DynamicsStage    fast, smooth;
    ReverbStage      reverb;
    OutputStage      output;

    ChainStatus status;
};

} // namespace gv
