#pragma once

#include "Biquad.h"

/*  OUTPUT: gain in dB followed by a brick-wall-ish safety limiter with a
    -0.3 dBFS ceiling. The limiter uses an instant attack / slow release gain
    smoother plus a final hard clamp, so no invalid sample can ever leave the
    plug-in.
*/
namespace gv
{

class OutputStage
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        gainSmooth.prepare (fs, 0.02);
        attackCoef  = (float) std::exp (-1.0 / std::max (1.0, fs * 0.0002));   // 0.2 ms
        releaseCoef = (float) std::exp (-1.0 / std::max (1.0, fs * 0.150));    // 150 ms
        limiterGain = 1.0f;
        reductionDb = 0.0f;
    }

    void reset()
    {
        gainSmooth.snap (1.0f);
        limiterGain = 1.0f;
        reductionDb = 0.0f;
        active = false;
    }

    void setGainDb (float db, bool snapNow)
    {
        const float g = dbToGain (db);
        gainSmooth.setTarget (g);
        if (snapNow) gainSmooth.snap (g);
    }

    void process (float* left, float* right, int numSamples)
    {
        constexpr float ceiling = 0.966051f;   // -0.3 dBFS
        float maxRed = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float g = gainSmooth.next();
            float l = left[i]  * g;
            float r = right[i] * g;

            const float peak = std::max (std::abs (l), std::abs (r));
            const float target = peak > ceiling ? ceiling / peak : 1.0f;

            // Instant attack (the ceiling must never be exceeded) with a smooth release.
            if (target < limiterGain)
                limiterGain = target;
            else
                limiterGain = target + releaseCoef * (limiterGain - target);

            l *= limiterGain;
            r *= limiterGain;

            // Guarantee valid, in-range output even if a host sends garbage in.
            if (! std::isfinite (l)) l = 0.0f;
            if (! std::isfinite (r)) r = 0.0f;
            l = std::clamp (l, -0.999f, 0.999f);
            r = std::clamp (r, -0.999f, 0.999f);

            left[i]  = l;
            right[i] = r;

            maxRed = std::max (maxRed, -gainToDb (limiterGain));
        }

        reductionDb = maxRed;
        active = maxRed > 0.1f;
    }

    float getReductionDb() const noexcept { return reductionDb; }
    bool  isActive() const noexcept       { return active; }

private:
    double fs = 44100.0;
    OnePole gainSmooth;
    float attackCoef = 0.0f, releaseCoef = 0.0f;
    float limiterGain = 1.0f, reductionDb = 0.0f;
    bool active = false;
};

} // namespace gv
