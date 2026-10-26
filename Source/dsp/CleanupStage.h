#pragma once

#include "Biquad.h"

/*  CLEAN-UP group: low cut, gate, mud reduction and grace presence.
    All four stages are stereo linked and completely allocation free once
    prepare() has been called.
*/
namespace gv
{

//==============================================================================
/** Low cut: one to three cascaded 2nd-order high-pass sections (12/24/36 dB/oct). */
class LowCutStage
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        freqSmooth.prepare (sampleRate, 0.02);
        for (auto& s : sections) s.reset();
        currentSlope = 0;
        coeffCounter = 0;
    }

    void reset() { for (auto& s : sections) s.reset(); }

    void setTarget (float hz, int slope)
    {
        freqSmooth.setTarget (hz);
        numSections = std::clamp (slope / 12, 1, 3);
    }

    void snap() { freqSmooth.snap (freqSmooth.getCurrent()); updateCoeffs(); }

    void process (float* left, float* right, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            if (--coeffCounter <= 0)
                updateCoeffs();

            float l = left[i], r = right[i];
            for (int s = 0; s < numSections; ++s)
            {
                l = sections[s].l.process (l);
                r = sections[s].r.process (r);
            }
            left[i] = l;
            right[i] = r;
        }
    }

private:
    void updateCoeffs()
    {
        coeffCounter = 32;
        const auto f = freqSmooth.next();
        if (numSections != currentSlope)
        {
            currentSlope = numSections;
            for (auto& s : sections) s.reset();
        }
        // Butterworth style Q for the cascaded sections keeps the knee clean.
        const auto c = BiquadCoeffs::highpass (fs, f, 0.7071);
        for (int s = 0; s < 3; ++s)
            sections[s].setCoeffs (c);
    }

    double fs = 44100.0;
    StereoBiquad sections[3];
    OnePole freqSmooth;
    int numSections = 3, currentSlope = -1, coeffCounter = 0;
};

//==============================================================================
/** Downward expander / noise gate with hysteresis and soft gain ramps. */
class GateStage
{
public:
    void prepare (double sampleRate)
    {
        detector.prepare (sampleRate, 0.001, 0.040);
        gainSmooth.prepare (sampleRate, 0.0015, 0.120);   // open fast, close gently
        gainSmooth.snap (1.0f);
        open = false;
        reductionDb = 0.0f;
    }

    void reset()
    {
        detector.reset();
        gainSmooth.snap (1.0f);
        open = false;
    }

    void setAmount (float percent) { amount = std::clamp (percent, 0.0f, 100.0f) / 100.0f; }

    void process (float* left, float* right, int numSamples)
    {
        // 0 %  -> -70 dB threshold and only 6 dB of attenuation (effectively off)
        // 100 % -> -20 dB threshold and 60 dB of attenuation
        const float thresholdDb = -70.0f + 50.0f * amount;
        const float attenDb     = -(6.0f + 54.0f * amount);
        const float attenGain   = dbToGain (attenDb);
        const float hysteresis  = 6.0f;

        float maxReduction = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float det = std::max (std::abs (left[i]), std::abs (right[i]));
            const float env = detector.process (det);
            const float envDb = gainToDb (env);

            if (open)
            {
                if (envDb < thresholdDb - hysteresis)
                    open = false;
            }
            else if (envDb > thresholdDb)
            {
                open = true;
            }

            const float target = open ? 1.0f : attenGain;
            const float g = gainSmooth.process (target);

            left[i]  *= g;
            right[i] *= g;

            maxReduction = std::max (maxReduction, -gainToDb (g));
        }

        reductionDb = maxReduction;
        active = maxReduction > 0.5f;
    }

    float getReductionDb() const noexcept { return reductionDb; }
    bool  isActive() const noexcept       { return active; }

private:
    /** Asymmetric one-pole used for the gate gain so it opens quickly and closes smoothly. */
    struct GateSmoother
    {
        void prepare (double fs, double openSec, double closeSec)
        {
            openCoef  = (float) std::exp (-1.0 / std::max (1.0, fs * openSec));
            closeCoef = (float) std::exp (-1.0 / std::max (1.0, fs * closeSec));
        }
        void snap (float v) { value = v; }
        inline float process (float target)
        {
            const float c = target > value ? openCoef : closeCoef;
            value = target + c * (value - target);
            return value;
        }
        float openCoef = 0.0f, closeCoef = 0.0f, value = 1.0f;
    };

    EnvelopeFollower detector;
    GateSmoother gainSmooth;
    float amount = 0.2f, reductionDb = 0.0f;
    bool open = false, active = false;
};

//==============================================================================
/** Mud reduction: dynamic attenuation of the 170-520 Hz "boxy" band. */
class MudStage
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        const auto cHi = BiquadCoeffs::lowpass (fs, 520.0, 0.7071);
        const auto cLo = BiquadCoeffs::lowpass (fs, 170.0, 0.7071);
        for (auto& b : lpHi) b.setCoeffs (cHi);
        for (auto& b : lpLo) b.setCoeffs (cLo);
        bandEnv.prepare (fs, 0.010, 0.120);
        cutSmooth.prepare (fs, 0.030);
        cutSmooth.snap (0.0f);
        reset();
    }

    void reset()
    {
        for (auto& b : lpHi) b.reset();
        for (auto& b : lpLo) b.reset();
        bandEnv.reset();
        cutSmooth.snap (0.0f);
    }

    void setAmount (float percent) { amount = std::clamp (percent, 0.0f, 100.0f) / 100.0f; }

    void process (float* left, float* right, int numSamples)
    {
        // Static part gives an immediate tonal result, the envelope driven part
        // keeps the reduction moving with the performance.
        const float staticCut = 0.60f * amount;
        const float dynamicCut = 0.40f * amount;

        float maxCut = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float bl = lpHi[0].process (left[i])  - lpLo[0].process (left[i]);
            const float br = lpHi[1].process (right[i]) - lpLo[1].process (right[i]);

            const float env = bandEnv.process (0.5f * (bl + br));
            // -42 dB .. -6 dB mapped to 0..1
            const float envNorm = std::clamp ((gainToDb (env) + 42.0f) / 36.0f, 0.0f, 1.0f);

            cutSmooth.setTarget (staticCut + dynamicCut * envNorm);
            const float cut = cutSmooth.next();

            left[i]  -= bl * cut;
            right[i] -= br * cut;
            maxCut = std::max (maxCut, cut);
        }

        // 0..1 cut -> roughly 0..-9 dB inside the band
        reductionDb = -9.0f * maxCut;
        active = maxCut > 0.06f;
    }

    float getReductionDb() const noexcept { return reductionDb; }
    bool  isActive() const noexcept       { return active; }

private:
    double fs = 44100.0;
    Biquad lpHi[2], lpLo[2];
    EnvelopeFollower bandEnv;
    OnePole cutSmooth;
    float amount = 0.25f, reductionDb = 0.0f;
    bool active = false;
};

//==============================================================================
/** Grace: controlled presence lift around 1.8 - 6.5 kHz. */
class GraceStage
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        const auto hp = BiquadCoeffs::highpass (fs, 1800.0, 0.7071);
        const auto lp = BiquadCoeffs::lowpass  (fs, 6500.0, 0.7071);
        hpL.setCoeffs (hp); hpR.setCoeffs (hp);
        lpL.setCoeffs (lp); lpR.setCoeffs (lp);
        mixSmooth.prepare (fs, 0.030);
        reset();
    }

    void reset()
    {
        hpL.reset(); hpR.reset(); lpL.reset(); lpR.reset();
        mixSmooth.snap (0.0f);
    }

    void setAmount (float percent) { target = std::clamp (percent, 0.0f, 100.0f) / 100.0f; }
    void setTargetNow() { mixSmooth.snap (target); }

    void process (float* left, float* right, int numSamples)
    {
        if (target < 0.001f && std::abs (mixSmooth.getCurrent()) < 1.0e-4f)
            return;

        mixSmooth.setTarget (target);

        for (int i = 0; i < numSamples; ++i)
        {
            const float m = mixSmooth.next() * 0.95f;
            const float presL = lpL.process (hpL.process (left[i]));
            const float presR = lpR.process (hpR.process (right[i]));
            left[i]  += presL * m;
            right[i] += presR * m;
        }
    }

private:
    double fs = 44100.0;
    Biquad hpL, hpR, lpL, lpR;
    OnePole mixSmooth;
    float target = 0.35f;
};

} // namespace gv
