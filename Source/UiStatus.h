#pragma once

/** Lock-free snapshot of the meter / indicator values the editor draws. */
struct UiStatus
{
    float inputPeak = 0.0f, outputPeak = 0.0f;
    float fastReductionDb = 0.0f, smoothReductionDb = 0.0f, limiterReductionDb = 0.0f;
    bool gateActive = false, mudActive = false, deEssActive = false, limiterActive = false;
};
