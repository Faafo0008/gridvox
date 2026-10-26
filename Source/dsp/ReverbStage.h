#pragma once

#include "Biquad.h"
#include <vector>

/*  SPACE: an 8-line feedback delay network with input diffusion.

    ROOM  - short, damped, narrow (small delay scaling, heavy damping)
    PLATE - dense, bright and even (medium scaling, light damping)
    HALL  - long, wide and dark (large scaling, strong damping)

    The decay time is set as an RT60 in seconds (tempo synced by the host), and
    the per-line feedback gains are derived from it so the loop can never run
    away: gains are additionally clamped and soft-clipped.
*/
namespace gv
{

class ReverbStage
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        const double scale = fs / 44100.0;
        maxLineLength = 0;

        for (int i = 0; i < numLines; ++i)
        {
            lineLength[i] = (int) std::round (baseLengths[i] * scale * 1.8);
            maxLineLength = std::max (maxLineLength, lineLength[i]);
        }

        for (auto& b : lines) b.assign ((size_t) maxLineLength + 4, 0.0f);
        for (auto& d : diffusers) d.buf.assign (4096, 0.0f);

        diffuseLengths[0] = (int) std::round (142.0 * scale);
        diffuseLengths[1] = (int) std::round ( 221.0 * scale);
        diffuseLengths[2] = (int) std::round ( 331.0 * scale);
        diffuseLengths[3] = (int) std::round ( 451.0 * scale);

        for (int i = 0; i < 4; ++i)
            diffuseLengths[i] = std::clamp (diffuseLengths[i], 8, 2040);

        loCutL.setCoeffs (BiquadCoeffs::highpass (fs, 320.0, 0.7071));
        loCutR.setCoeffs (BiquadCoeffs::highpass (fs, 320.0, 0.7071));
        hiCutL.setCoeffs (BiquadCoeffs::lowpass (fs, 8000.0, 0.7071));
        hiCutR.setCoeffs (BiquadCoeffs::lowpass (fs, 8000.0, 0.7071));
        mixSmooth.prepare (fs, 0.04);
        reset();
    }

    void reset()
    {
        for (auto& b : lines) std::fill (b.begin(), b.end(), 0.0f);
        for (auto& d : diffusers) { std::fill (d.buf.begin(), d.buf.end(), 0.0f); d.index = 0; }
        for (auto& w : lineFilterState) w = 0.0f;
        for (int i = 0; i < numLines; ++i) writeIndex[i] = 0;
        loCutL.reset(); loCutR.reset(); hiCutL.reset(); hiCutR.reset();
        mixSmooth.snap (0.0f);
    }

    /** decaySeconds is an RT60, type is 0 room / 1 plate / 2 hall. */
    void setParams (float mixPercent, int type, float decaySeconds, float loCutHz, float hiCutHz)
    {
        mixTarget = std::clamp (mixPercent, 0.0f, 100.0f) / 100.0f;
        mode = std::clamp (type, 0, 2);
        decay = std::clamp (decaySeconds, 0.05f, 12.0f);

        if (loCutHz != lastLoCut)
        {
            lastLoCut = loCutHz;
            loCutL.setCoeffs (BiquadCoeffs::highpass (fs, loCutHz, 0.7071));
            loCutR.setCoeffs (BiquadCoeffs::highpass (fs, loCutHz, 0.7071));
        }
        if (hiCutHz != lastHiCut)
        {
            lastHiCut = hiCutHz;
            hiCutL.setCoeffs (BiquadCoeffs::lowpass (fs, hiCutHz, 0.7071));
            hiCutR.setCoeffs (BiquadCoeffs::lowpass (fs, hiCutHz, 0.7071));
        }
    }

    void process (float* left, float* right, int numSamples)
    {
        static constexpr double scaling[3]   = { 0.55, 1.00, 1.70 };
        static constexpr float  damping[3]  = { 0.62f, 0.30f, 0.70f };
        static constexpr float  diffusion[3]= { 0.45f, 0.62f, 0.55f };
        static constexpr float  width[3]    = { 0.55f, 0.85f, 1.00f };

        mixSmooth.setTarget (mixTarget);

        const double scale = scaling[mode];
        const float damp = damping[mode];
        const float diff = diffusion[mode];

        // Per line length + feedback gain for the requested RT60.
        for (int i = 0; i < numLines; ++i)
        {
            const int len = std::max (8, (int) std::round (lineLength[i] * scale));
            activeLength[i] = std::min (len, maxLineLength);
            const double rt60 = decay * (mode == 0 ? 0.8 : (mode == 1 ? 1.0 : 1.25));
            feedback[i] = (float) std::clamp (std::pow (10.0, -3.0 * activeLength[i] / (rt60 * fs)), 0.0, 0.95);
        }

        for (int n = 0; n < numSamples; ++n)
        {
            const float mix = mixSmooth.next();
            const float dryL = left[n], dryR = right[n];
            float in = 0.5f * (dryL + dryR);

            // ---- input diffusion (4 series all-passes) ----
            for (int d = 0; d < 4; ++d)
            {
                auto& ap = diffusers[d];
                const int len = diffuseLengths[d];
                const float delayed = ap.buf[(size_t) ((ap.index - len + 4096) % 4096)];
                const float w = in + delayed * diff;
                ap.buf[(size_t) ap.index] = w;
                ap.index = (ap.index + 1) % 4096;
                in = delayed - w * diff;
            }

            // ---- read the delay lines ----
            float taps[numLines];
            for (int i = 0; i < numLines; ++i)
            {
                const int idx = (writeIndex[i] - activeLength[i] + (int) lines[i].size()) % (int) lines[i].size();
                taps[i] = lines[i][(size_t) idx];

                // one-pole damping inside the loop
                lineFilterState[i] += damp * (taps[i] - lineFilterState[i]);
                taps[i] = lineFilterState[i];
            }

            // ---- 8 point Hadamard mixing (orthonormal) ----
            float mixed[numLines];
            for (int i = 0; i < numLines; ++i) mixed[i] = taps[i];

            for (int stage = 1; stage < numLines; stage <<= 1)
            {
                for (int i = 0; i < numLines; i += stage << 1)
                {
                    for (int j = 0; j < stage; ++j)
                    {
                        const float a = mixed[i + j], b = mixed[i + j + stage];
                        mixed[i + j]         = a + b;
                        mixed[i + j + stage] = a - b;
                    }
                }
            }

            const float norm = 1.0f / std::sqrt ((float) numLines);
            for (int i = 0; i < numLines; ++i)
                mixed[i] *= norm * feedback[i];

            // ---- write back with the input injected ----
            for (int i = 0; i < numLines; ++i)
            {
                const float inject = (i & 1) ? -in : in;
                float v = mixed[i] + inject * 0.35f;
                // Safety soft clip keeps the loop bounded even with extreme settings.
                v = std::clamp (v, -4.0f, 4.0f);
                lines[i][(size_t) writeIndex[i]] = v;
                writeIndex[i] = (writeIndex[i] + 1) % (int) lines[i].size();
            }

            // ---- stereo wet output ----
            float wetL = taps[0] - taps[2] + taps[4] - taps[6];
            float wetR = taps[1] - taps[3] + taps[5] - taps[7];
            wetL *= 0.35f;
            wetR *= 0.35f;

            wetL = hiCutL.process (loCutL.process (wetL));
            wetR = hiCutR.process (loCutR.process (wetR));

            const float w = mix * 0.95f;
            const float spread = width[mode];
            const float mid = 0.5f * (wetL + wetR);
            wetL = mid + (wetL - mid) * spread;
            wetR = mid + (wetR - mid) * spread;

            left[n]  = dryL  + wetL * w;
            right[n] = dryR + wetR * w;
        }
    }

private:
    static constexpr int numLines = 8;
    static constexpr double baseLengths[numLines] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };

    struct Diffuser { std::vector<float> buf; int index = 0; };

    double fs = 44100.0;
    std::vector<float> lines[numLines];
    int lineLength[numLines] {}, activeLength[numLines] {}, writeIndex[numLines] {};
    int maxLineLength = 0;

    Diffuser diffusers[4];
    int diffuseLengths[4] {};

    float lineFilterState[numLines] {}, feedback[numLines] {};
    Biquad loCutL, loCutR, hiCutL, hiCutR;
    OnePole mixSmooth;

    float mixTarget = 0.45f, decay = 0.46f, lastLoCut = -1.0f, lastHiCut = -1.0f;
    int mode = 1;
};

} // namespace gv
