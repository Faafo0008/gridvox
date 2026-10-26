#pragma once

#include "Biquad.h"

/*  TONE EQ: four stereo-linked peaking bands. Coefficients are refreshed every
    32 samples from smoothed gain/frequency values, which keeps knob moves free
    of clicks and is cheap enough for the audio thread.
*/
namespace gv
{

class EqStage
{
public:
    static constexpr int numBands = 4;
    static constexpr int coeffInterval = 32;

    void prepare (double sampleRate)
    {
        fs = sampleRate;
        for (int b = 0; b < numBands; ++b)
        {
            gainSmoothed[b].prepare (sampleRate, 0.03);
            freqSmoothed[b].prepare (sampleRate, 0.03);
            gainSmoothed[b].snap (0.0f);
            freqSmoothed[b].snap (1000.0f);
        }
        counter = 0;
        reset();
    }

    void reset()
    {
        for (auto& band : bands) band.reset();
    }

    void setParams (const float (&gainsDb)[numBands], const float (&freqsHz)[numBands], bool snapNow)
    {
        for (int b = 0; b < numBands; ++b)
        {
            gainSmoothed[b].setTarget (gainsDb[b]);
            freqSmoothed[b].setTarget (freqsHz[b]);
            if (snapNow)
            {
                gainSmoothed[b].snap (gainsDb[b]);
                freqSmoothed[b].snap (freqsHz[b]);
            }
        }
        if (snapNow) updateCoeffs();
    }

    void process (float* left, float* right, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            if (--counter <= 0)
                updateCoeffs();

            float l = left[i], r = right[i];
            for (auto& band : bands)
            {
                l = band.l.process (l);
                r = band.r.process (r);
            }
            left[i] = l;
            right[i] = r;
        }
    }

private:
    void updateCoeffs()
    {
        counter = coeffInterval;
        static constexpr double qValues[numBands] = { 0.70, 1.00, 1.00, 0.70 };

        for (int b = 0; b < numBands; ++b)
        {
            const float g = gainSmoothed[b].next();
            const float f = freqSmoothed[b].next();
            bands[b].setCoeffs (BiquadCoeffs::peak (fs, f, qValues[b], (double) g));
        }
    }

    double fs = 44100.0;
    StereoBiquad bands[numBands];
    OnePole gainSmoothed[numBands], freqSmoothed[numBands];
    int counter = 0;
};

} // namespace gv
