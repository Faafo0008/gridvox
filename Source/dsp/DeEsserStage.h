#pragma once

#include "Biquad.h"

/*  De-esser: a 4.5 kHz+ band is split off, an envelope follower drives gain
    reduction on that band only, so the body of the vocal stays untouched.
*/
namespace gv
{

class DeEsserStage
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        const auto hp = BiquadCoeffs::highpass (fs, 4500.0, 0.7071);
        hpL.setCoeffs (hp); hpR.setCoeffs (hp);
        detector.prepare (fs, 0.001, 0.030);
        gainSmooth.prepare (fs, 0.001, 0.060);
        gainSmooth.snap (1.0f);
        reset();
    }

    void reset()
    {
        hpL.reset(); hpR.reset();
        detector.reset();
        gainSmooth.snap (1.0f);
        reductionDb = 0.0f;
    }

    void setAmount (float percent) { amount = std::clamp (percent, 0.0f, 100.0f) / 100.0f; }

    void process (float* left, float* right, int numSamples)
    {
        if (amount < 0.001f)
        {
            reductionDb = 0.0f;
            active = false;
            return;
        }

        const float thresholdDb = -36.0f + 10.0f * amount;    // 0 %..100 % -> -36..-26 dB
        const float maxReductionDb = 12.0f * amount;

        float maxRed = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float sl = hpL.process (left[i]);
            const float sr = hpR.process (right[i]);

            const float env = detector.process (0.5f * (sl + sr));
            const float envDb = gainToDb (env);

            float redDb = 0.0f;
            if (envDb > thresholdDb)
                redDb = std::min (maxReductionDb, (envDb - thresholdDb) * 0.65f);

            const float g = gainSmooth.process (dbToGain (-redDb));

            left[i]  = (left[i]  - sl) + sl * g;
            right[i] = (right[i] - sr) + sr * g;

            maxRed = std::max (maxRed, -gainToDb (g));
        }

        reductionDb = maxRed;
        active = maxRed > 0.3f;
    }

    float getReductionDb() const noexcept { return reductionDb; }
    bool  isActive() const noexcept       { return active; }

private:
    double fs = 44100.0;
    Biquad hpL, hpR;
    EnvelopeFollower detector;
    AsymSmoother gainSmooth;
    float amount = 0.3f, reductionDb = 0.0f;
    bool active = false;
};

} // namespace gv
