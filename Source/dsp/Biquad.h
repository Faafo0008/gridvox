#pragma once

#include <cmath>
#include <algorithm>

/*  Minimal, allocation-free biquad (RBJ cookbook coefficients, transposed direct
    form II). Designed to be used per-sample inside the audio callback.
*/
namespace gv
{

struct BiquadCoeffs
{
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;

    static BiquadCoeffs lowpass (double fs, double f, double q = 0.70710678)  { return make (fs, f, q, 0.0, 0); }
    static BiquadCoeffs highpass (double fs, double f, double q = 0.70710678) { return make (fs, f, q, 0.0, 1); }
    static BiquadCoeffs peak (double fs, double f, double q, double gainDb)   { return make (fs, f, q, gainDb, 2); }
    static BiquadCoeffs lowShelf (double fs, double f, double gainDb, double s = 1.0)  { return shelf (fs, f, gainDb, s, true); }
    static BiquadCoeffs highShelf (double fs, double f, double gainDb, double s = 1.0) { return shelf (fs, f, gainDb, s, false); }

private:
    static BiquadCoeffs make (double fs, double f, double q, double gainDb, int type)
    {
        BiquadCoeffs c;
        f = std::clamp (f, 10.0, fs * 0.49);
        const double w0    = 2.0 * 3.14159265358979323846 * f / fs;
        const double cosw  = std::cos (w0);
        const double sinw  = std::sin (w0);
        const double alpha = sinw / (2.0 * std::max (0.05, q));
        const double A     = std::pow (10.0, gainDb / 40.0);

        double b0 = 0, b1 = 0, b2 = 0, a0 = 0, a1 = 0, a2 = 0;

        switch (type)
        {
            case 0: // low pass
                b0 = (1.0 - cosw) * 0.5; b1 = 1.0 - cosw; b2 = b0;
                a0 = 1.0 + alpha;        a1 = -2.0 * cosw; a2 = 1.0 - alpha;
                break;
            case 1: // high pass
                b0 = (1.0 + cosw) * 0.5; b1 = -(1.0 + cosw); b2 = b0;
                a0 = 1.0 + alpha;         a1 = -2.0 * cosw;  a2 = 1.0 - alpha;
                break;
            default: // peaking
                b0 = 1.0 + alpha * A; b1 = -2.0 * cosw; b2 = 1.0 - alpha * A;
                a0 = 1.0 + alpha / A; a1 = -2.0 * cosw; a2 = 1.0 - alpha / A;
                break;
        }

        c.b0 = (float) (b0 / a0); c.b1 = (float) (b1 / a0); c.b2 = (float) (b2 / a0);
        c.a1 = (float) (a1 / a0); c.a2 = (float) (a2 / a0);
        return c;
    }

    static BiquadCoeffs shelf (double fs, double f, double gainDb, double s, bool low)
    {
        BiquadCoeffs c;
        f = std::clamp (f, 10.0, fs * 0.49);
        const double A     = std::pow (10.0, gainDb / 40.0);
        const double w0    = 2.0 * 3.14159265358979323846 * f / fs;
        const double cosw  = std::cos (w0);
        const double sinw  = std::sin (w0);
        const double beta  = std::sqrt (std::max (0.0, (A * A + 1.0) / s - (A - 1.0) * (A - 1.0)));
        const double alpha = sinw / 2.0 * beta;
        const double ap1   = A + 1.0, am1 = A - 1.0, sq = 2.0 * std::sqrt (A) * alpha;

        double b0, b1, b2, a0, a1, a2;

        if (low)
        {
            b0 = A * (ap1 - am1 * cosw + sq); b1 = 2.0 * A * (am1 - ap1 * cosw); b2 = A * (ap1 - am1 * cosw - sq);
            a0 = ap1 + am1 * cosw + sq;       a1 = -2.0 * (am1 + ap1 * cosw);    a2 = ap1 + am1 * cosw - sq;
        }
        else
        {
            b0 = A * (ap1 + am1 * cosw + sq); b1 = -2.0 * A * (am1 + ap1 * cosw); b2 = A * (ap1 + am1 * cosw - sq);
            a0 = ap1 - am1 * cosw + sq;       a1 = 2.0 * (am1 - ap1 * cosw);      a2 = ap1 - am1 * cosw - sq;
        }

        c.b0 = (float) (b0 / a0); c.b1 = (float) (b1 / a0); c.b2 = (float) (b2 / a0);
        c.a1 = (float) (a1 / a0); c.a2 = (float) (a2 / a0);
        return c;
    }
};

/** A single biquad section. */
class Biquad
{
public:
    void reset() noexcept { z1 = z2 = 0.0f; }

    void setCoeffs (const BiquadCoeffs& nc) noexcept { c = nc; }
    const BiquadCoeffs& coeffs() const noexcept { return c; }

    inline float process (float x) noexcept
    {
        const float y = c.b0 * x + z1;
        z1 = c.b1 * x - c.a1 * y + z2;
        z2 = c.b2 * x - c.a2 * y;
        // Flush denormals so a silent input cannot spike the CPU.
        z1 = flush (z1);
        z2 = flush (z2);
        return y;
    }

private:
    static inline float flush (float v) noexcept
    {
        return (std::abs (v) < 1.0e-20f) ? 0.0f : v;
    }

    BiquadCoeffs c {};
    float z1 = 0.0f, z2 = 0.0f;
};

/** Stereo pair of a stereo-linked filter. */
struct StereoBiquad
{
    Biquad l, r;
    void reset() noexcept { l.reset(); r.reset(); }
    void setCoeffs (const BiquadCoeffs& c) noexcept { l.setCoeffs (c); r.setCoeffs (c); }
};

/** One-pole smoothing used for parameter ramps (avoids zipper noise). */
class OnePole
{
public:
    void prepare (double sampleRate, double timeSeconds) noexcept
    {
        coef = (float) std::exp (-1.0 / std::max (1.0, sampleRate * timeSeconds));
    }

    void setTarget (float t) noexcept { target = t; }
    void snap (float v) noexcept { value = target = v; }

    inline float next() noexcept
    {
        value = target + coef * (value - target);
        return value;
    }

    float getCurrent() const noexcept { return value; }

private:
    float coef = 0.0f, value = 0.0f, target = 0.0f;
};

/** Asymmetric one-pole smoother (different rise/fall times), used for gain ramps. */
class AsymSmoother
{
public:
    void prepare (double sampleRate, double riseSec, double fallSec) noexcept
    {
        riseCoef = (float) std::exp (-1.0 / std::max (1.0, sampleRate * riseSec));
        fallCoef = (float) std::exp (-1.0 / std::max (1.0, sampleRate * fallSec));
    }

    void snap (float v) noexcept { value = v; }

    inline float process (float target) noexcept
    {
        const float c = target > value ? riseCoef : fallCoef;
        value = target + c * (value - target);
        return value;
    }

    float getCurrent() const noexcept { return value; }

private:
    float riseCoef = 0.0f, fallCoef = 0.0f, value = 0.0f;
};

/** Simple peak/RMS envelope follower with independent attack and release. */
class EnvelopeFollower
{
public:
    void prepare (double sampleRate, double attackSec, double releaseSec) noexcept
    {
        att = (float) std::exp (-1.0 / std::max (1.0, sampleRate * attackSec));
        rel = (float) std::exp (-1.0 / std::max (1.0, sampleRate * releaseSec));
        env = 0.0f;
    }

    inline float process (float x) noexcept
    {
        const float a = std::abs (x);
        env = a > env ? att * (env - a) + a : rel * (env - a) + a;
        return env;
    }

    float get() const noexcept { return env; }
    void reset() noexcept { env = 0.0f; }

private:
    float att = 0.0f, rel = 0.0f, env = 0.0f;
};

inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
inline float gainToDb (float g) noexcept { return 20.0f * std::log10 (std::max (1.0e-9f, g)); }

} // namespace gv
