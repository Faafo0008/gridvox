#pragma once

#include "Biquad.h"
#include <cstring>

/*  Warmth: 2x oversampled soft saturation.

    The oversampler is a fixed 17-tap half-band FIR (linear phase, delay of
    8 samples at 2x rate = 4 samples at the base rate per filter, 8 samples for
    the complete up/down pair). That latency is reported to the host.
*/
namespace gv
{

class Oversampler2x
{
public:
    void prepare (double /*sampleRate*/)
    {
        // Half-band design: h[n] = 0.5 * sinc(0.5 * (n - 8)) * Blackman(n)
        constexpr int M = 8;
        double h[17] {};
        double sum = 0.0;

        for (int n = 0; n < 17; ++n)
        {
            const double t = (double) (n - M) * 0.5;
            const double sinc = (std::abs (t) < 1.0e-9) ? 1.0 : std::sin (3.14159265358979323846 * t)
                                                            / (3.14159265358979323846 * t);
            const double w = 0.42 - 0.5 * std::cos (2.0 * 3.14159265358979323846 * n / 16.0)
                                  + 0.08 * std::cos (4.0 * 3.14159265358979323846 * n / 16.0);
            h[n] = 0.5 * sinc * w;
            sum += h[n];
        }

        for (int n = 0; n < 17; ++n)
        {
            downCoeffs[n] = (float) (h[n] / sum);          // unity DC gain
            upCoeffs[n]   = (float) (2.0 * h[n] / sum);    // compensates zero stuffing
        }

        reset();
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            std::memset (upHistory[ch], 0, sizeof (upHistory[ch]));
            std::memset (downHistory[ch], 0, sizeof (downHistory[ch]));
            upIndex[ch] = 0;
            downIndex[ch] = 0;
        }
    }

    /** Reads one base-rate sample, returns the pair of 2x-rate samples (in-place). */
    inline void upsample (int ch, float x, float& out0, float& out1)
    {
        float* h = upHistory[ch];
        int& idx = upIndex[ch];

        // sub-sample 0: zero-stuffed input (x * 2), sub-sample 1: zero
        for (int sub = 0; sub < 2; ++sub)
        {
            h[idx] = (sub == 0) ? x * 2.0f : 0.0f;
            idx = (idx + 1) & 31;

            float acc = 0.0f;
            int p = idx;
            for (int k = 0; k < 17; ++k)
            {
                p = (p - 1) & 31;
                acc += upCoeffs[k] * h[p];
            }

            (sub == 0 ? out0 : out1) = acc;
        }
    }

    inline float downsample (int ch, float in0, float in1)
    {
        float* h = downHistory[ch];
        int& idx = downIndex[ch];
        float result = 0.0f;

        for (int sub = 0; sub < 2; ++sub)
        {
            h[idx] = (sub == 0) ? in0 : in1;
            idx = (idx + 1) & 31;

            float acc = 0.0f;
            int p = idx;
            for (int k = 0; k < 17; ++k)
            {
                p = (p - 1) & 31;
                acc += downCoeffs[k] * h[p];
            }
            result = acc;
        }

        return result;
    }

    static constexpr int latencySamples = 8;   // 2 filters x 4 samples at base rate

private:
    float upCoeffs[17] {}, downCoeffs[17] {};
    float upHistory[2][32] {};
    float downHistory[2][32] {};
    int upIndex[2] {}, downIndex[2] {};
};

//==============================================================================
class WarmthStage
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        oversampler.prepare (sampleRate);
        mixSmooth.prepare (sampleRate, 0.03);
        const auto hpf = BiquadCoeffs::highpass (sampleRate, 12.0, 0.7071);
        dcL.setCoeffs (hpf);
        dcR.setCoeffs (hpf);
        reset();
    }

    void reset()
    {
        oversampler.reset();
        dcL.reset();
        dcR.reset();
        mixSmooth.snap (0.0f);
    }

    void setAmount (float percent) { amount = std::clamp (percent, 0.0f, 100.0f) / 100.0f; }

    void process (float* left, float* right, int numSamples)
    {
        // At 0 % the stage is a straight wire.
        if (amount < 0.001f)
        {
            return;
        }

        const float drive = 1.0f + 5.0f * amount;
        const float invDrive = 1.0f / drive;
        mixSmooth.setTarget (amount);

        for (int i = 0; i < numSamples; ++i)
        {
            const float m = mixSmooth.next();

            float l0, l1, r0, r1;
            oversampler.upsample (0, left[i], l0, l1);
            oversampler.upsample (1, right[i], r0, r1);

            // tanh() normalised for unity small-signal gain keeps the level stable.
            l0 = std::tanh (drive * l0) * invDrive;
            l1 = std::tanh (drive * l1) * invDrive;
            r0 = std::tanh (drive * r0) * invDrive;
            r1 = std::tanh (drive * r1) * invDrive;

            const float yl = oversampler.downsample (0, l0, l1);
            const float yr = oversampler.downsample (1, r0, r1);

            left[i]  = dcL.process (left[i]  + (yl - left[i])  * m);
            right[i] = dcR.process (right[i] + (yr - right[i]) * m);
        }
    }

    static constexpr int latencySamples = Oversampler2x::latencySamples;

private:
    double fs = 44100.0;
    Oversampler2x oversampler;
    OnePole mixSmooth;
    Biquad dcL, dcR;
    float amount = 0.2f;
};

} // namespace gv
