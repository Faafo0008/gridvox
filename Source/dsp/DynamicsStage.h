#pragma once

#include "Biquad.h"

/*  Stereo-linked soft-knee compressor used for both the FAST (peak control) and
    SMOOTH (gentle levelling) stages. The "amount" control scales the ratio from
    1:1 (inaudible) up to the stage maximum, so 0 % is a true bypass.
*/
namespace gv
{

class DynamicsStage
{
public:
    struct Config
    {
        float thresholdDb;
        float maxRatio;
        double attackMs;
        double releaseMs;
        float kneeDb;
    };

    void prepare (double sampleRate, const Config& c)
    {
        fs = sampleRate;
        cfg = c;
        attackCoef  = (float) std::exp (-1.0 / std::max (1.0, fs * cfg.attackMs  * 0.001));
        releaseCoef = (float) std::exp (-1.0 / std::max (1.0, fs * cfg.releaseMs * 0.001));
        reset();
    }

    void reset()
    {
        smoothedGain = 1.0f;
        reductionDb = 0.0f;
        active = false;
    }

    void setAmount (float percent) { amount = std::clamp (percent, 0.0f, 100.0f) / 100.0f; }

    void process (float* left, float* right, int numSamples)
    {
        if (amount < 0.0005f)
        {
            reductionDb = 0.0f;
            active = false;
            return;
        }

        const float ratio = 1.0f + (cfg.maxRatio - 1.0f) * amount;
        const float halfKnee = cfg.kneeDb * 0.5f;
        float maxRed = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float peak = std::max (std::abs (left[i]), std::abs (right[i]));
            const float levelDb = gainToDb (peak);
            const float overDb = levelDb - cfg.thresholdDb;

            float targetRedDb = 0.0f;

            if (overDb > halfKnee)
                targetRedDb = overDb - overDb / ratio;
            else if (overDb > -halfKnee)
            {
                const float x = overDb + halfKnee;
                const float soft = (x * x) / (2.0f * cfg.kneeDb);
                targetRedDb = soft - soft / ratio;
            }

            const float targetGain = dbToGain (-targetRedDb);
            const float coef = targetGain < smoothedGain ? attackCoef : releaseCoef;
            smoothedGain = targetGain + coef * (smoothedGain - targetGain);

            left[i]  *= smoothedGain;
            right[i] *= smoothedGain;

            maxRed = std::max (maxRed, -gainToDb (smoothedGain));
        }

        reductionDb = maxRed;
        active = maxRed > 0.2f;
    }

    float getReductionDb() const noexcept { return reductionDb; }
    bool  isActive() const noexcept       { return active; }

private:
    double fs = 44100.0;
    Config cfg { -18.0f, 4.0f, 5.0, 80.0, 6.0f };
    float attackCoef = 0.0f, releaseCoef = 0.0f;
    float smoothedGain = 1.0f;
    float amount = 0.15f, reductionDb = 0.0f;
    bool active = false;
};

} // namespace gv
