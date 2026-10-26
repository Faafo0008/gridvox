/*
    Offline verification harness for the GRID VOX DSP chain.

    Runs the real GridVoxProcessor through a set of signals and checks:
      * no NaN / Inf / out of range samples
      * no runaway gain (gate / limiter stability)
      * mono and stereo behaviour
      * sample-rate independence
      * bypass transparency
      * preset switching and state save / restore
      * rough CPU cost per block

    Exit code 0 means every check passed.
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "presets/Presets.h"
#include "params/ParameterIDs.h"

#include <cstdio>
#include <random>
#include <chrono>

namespace
{
    int failures = 0;

    void check (bool condition, const juce::String& what)
    {
        std::printf ("  [%s] %s\n", condition ? "PASS" : "FAIL", what.toRawUTF8());
        if (! condition)
            ++failures;
    }

    struct SignalStats
    {
        float peak = 0.0f;
        double rms = 0.0;
        bool allFinite = true;
        bool inRange = true;
    };

    SignalStats analyse (const juce::AudioBuffer<float>& buffer)
    {
        SignalStats s;
        double sum = 0.0;
        juce::int64 count = 0;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const auto* d = buffer.getReadPointer (ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const float v = d[i];
                if (! std::isfinite (v)) { s.allFinite = false; continue; }
                if (std::abs (v) > 1.0f) s.inRange = false;
                s.peak = juce::jmax (s.peak, std::abs (v));
                sum += (double) v * (double) v;
                ++count;
            }
        }

        s.rms = count > 0 ? std::sqrt (sum / (double) count) : 0.0;
        return s;
    }

    /** Fills a buffer with the requested test signal. */
    void fill (juce::AudioBuffer<float>& buffer, int kind, double sampleRate, juce::Random& rng)
    {
        const int n = buffer.getNumSamples();
        const int channels = buffer.getNumChannels();

        for (int ch = 0; ch < channels; ++ch)
        {
            auto* d = buffer.getWritePointer (ch);
            for (int i = 0; i < n; ++i)
            {
                switch (kind)
                {
                    case 0: d[i] = 0.0f; break;                                       // silence
                    case 1: d[i] = (rng.nextFloat() * 2.0f - 1.0f) * 0.0006f; break;   // very quiet noise
                    case 2: d[i] = 0.5f * std::sin (2.0 * 3.14159265358979323846 * 440.0 * i / sampleRate); break;
                    case 3: d[i] = 0.9f * std::sin (2.0 * 3.14159265358979323846 * 120.0 * i / sampleRate); break;
                    case 4: d[i] = (rng.nextFloat() * 2.0f - 1.0f) * 0.7f; break;      // loud noise
                    case 5: d[i] = (i == 0) ? 1.0f : 0.0f; break;                      // impulse
                    case 6: d[i] = 0.6f * std::sin (2.0 * 3.14159265358979323846 * 8000.0 * i / sampleRate); break; // sibilance
                    default: d[i] = 0.0f;
                }
            }
        }
    }

    void runSignal (const juce::String& name, GridVoxProcessor& processor, int signalKind,
                    double sampleRate, int blockSize, int numBlocks, int channels)
    {
        processor.prepareToPlay (sampleRate, blockSize);
        processor.setPlayConfigDetails (channels, channels, sampleRate, blockSize);

        juce::Random rng (1234);
        juce::AudioBuffer<float> buffer (channels, blockSize);
        juce::MidiBuffer midi;

        SignalStats worst;

        for (int b = 0; b < numBlocks; ++b)
        {
            fill (buffer, signalKind, sampleRate, rng);
            midi.clear();
            processor.processBlock (buffer, midi);

            const auto s = analyse (buffer);
            worst.peak = juce::jmax (worst.peak, s.peak);
            worst.allFinite = worst.allFinite && s.allFinite;
            worst.inRange = worst.inRange && s.inRange;
        }

        check (worst.allFinite, name + ": all samples finite");
        check (worst.inRange, name + ": |output| <= 1.0 (peak " + juce::String (worst.peak, 3) + ")");
        check (worst.peak < 0.98f, name + ": no clipping (peak " + juce::String (worst.peak, 3) + ")");
    }
}

//==============================================================================
int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    juce::ignoreUnused (argc, argv);

    std::printf ("GRID VOX DSP verification\n=========================\n");

    // ---------------------------------------------------------------------
    std::printf ("\n1) Stability across sample rates / block sizes / layouts\n");
    struct Case { double sr; int block; int channels; const char* name; };
    const Case cases[] = {
        { 44100.0, 512, 2, "44.1 kHz stereo 512f" },
        { 48000.0, 128, 2, "48 kHz stereo 128f" },
        { 96000.0, 64,  2, "96 kHz stereo 64f" },
        { 192000.0, 256, 2, "192 kHz stereo 256f" },
        { 48000.0, 512, 1, "48 kHz mono 512f" }
    };

    for (const auto& c : cases)
    {
        GridVoxProcessor processor;
        runSignal (juce::String (c.name) + " / silence",     processor, 0, c.sr, c.block, 40, c.channels);
        runSignal (juce::String (c.name) + " / -64 dB noise", processor, 1, c.sr, c.block, 40, c.channels);
        runSignal (juce::String (c.name) + " / sine 440",    processor, 2, c.sr, c.block, 40, c.channels);
        runSignal (juce::String (c.name) + " / loud noise",  processor, 4, c.sr, c.block, 60, c.channels);
        runSignal (juce::String (c.name) + " / impulse",     processor, 5, c.sr, c.block, 20, c.channels);
    }

    // ---------------------------------------------------------------------
    std::printf ("\n2) Gate / low-level stability\n");
    {
        GridVoxProcessor processor;
        processor.prepareToPlay (48000.0, 256);
        juce::AudioBuffer<float> buffer (2, 256);
        juce::MidiBuffer midi;
        juce::Random rng (99);

        float maxPeak = 0.0f;
        for (int b = 0; b < 400; ++b)
        {
            fill (buffer, 1, 48000.0, rng);        // -64 dBFS noise floor
            processor.processBlock (buffer, midi);
            maxPeak = juce::jmax (maxPeak, analyse (buffer).peak);
        }
        check (maxPeak < 0.05f, "low-level input stays quiet (peak " + juce::String (maxPeak, 4) + ")");
        check (analyse (buffer).allFinite, "low-level input produces finite output");
    }

    // ---------------------------------------------------------------------
    std::printf ("\n3) Bypass transparency\n");
    {
        GridVoxProcessor processor;
        processor.prepareToPlay (48000.0, 256);
        if (auto* p = processor.apvts.getParameter (ParamIDs::bypass))
            p->setValueNotifyingHost (1.0f);

        juce::AudioBuffer<float> buffer (2, 256);
        juce::AudioBuffer<float> reference (2, 256);
        juce::MidiBuffer midi;
        juce::Random rng (7);

        bool identical = true;
        for (int b = 0; b < 10; ++b)
        {
            fill (buffer, 2, 48000.0, rng);
            reference.makeCopyOf (buffer);
            processor.processBlock (buffer, midi);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 256; ++i)
                    if (std::abs (buffer.getSample (ch, i) - reference.getSample (ch, i)) > 1.0e-6f)
                        identical = false;
        }
        check (identical, "bypassed output equals input");
    }

    // ---------------------------------------------------------------------
    std::printf ("\n4) Presets\n");
    {
        GridVoxProcessor processor;
        processor.prepareToPlay (48000.0, 256);

        const auto& list = Presets::all();
        check (list.size() >= 5, "at least five factory presets (" + juce::String ((int) list.size()) + ")");
        check (processor.getCurrentPresetName() == "CLEAN TRACKING", "default preset is CLEAN TRACKING");

        const float gateBefore = processor.apvts.getRawParameterValue (ParamIDs::gate)->load();
        processor.setCurrentPreset (Presets::indexOf ("BROADCAST"));
        const float gateAfter = processor.apvts.getRawParameterValue (ParamIDs::gate)->load();
        check (processor.getCurrentPresetName() == "BROADCAST", "preset name follows selection");
        check (std::abs (gateBefore - gateAfter) > 0.01f, "preset switching updates parameters");

        processor.nextPreset();
        const auto nextName = processor.getCurrentPresetName();
        processor.previousPreset();
        check (nextName != processor.getCurrentPresetName(), "next / previous preset navigation works");

        bool allApplied = true;
        for (int i = 0; i < (int) list.size(); ++i)
        {
            processor.setCurrentPreset (i);
            for (const auto& kv : list[(size_t) i].values)
                if (auto* p = processor.apvts.getParameter (kv.first))
                {
                    const float actual = p->convertFrom0to1 (p->getValue());
                    if (std::abs (actual - kv.second) > 0.51f)
                    {
                        std::printf ("      mismatch %s: %f vs %f\n", kv.first, actual, kv.second);
                        allApplied = false;
                    }
                }
        }
        check (allApplied, "every preset value applied exactly");
    }

    // ---------------------------------------------------------------------
    std::printf ("\n5) State save / restore\n");
    {
        GridVoxProcessor processor;
        processor.prepareToPlay (48000.0, 256);
        processor.setCurrentPreset (Presets::indexOf ("BRIGHT VOCAL"));

        if (auto* p = processor.apvts.getParameter (ParamIDs::output))
            p->setValueNotifyingHost (p->convertTo0to1 (-3.0f));

        juce::MemoryBlock state;
        processor.getStateInformation (state);

        GridVoxProcessor restored;
        restored.prepareToPlay (48000.0, 256);
        restored.setStateInformation (state.getData(), (int) state.getSize());

        const float out = restored.apvts.getRawParameterValue (ParamIDs::output)->load();
        check (std::abs (out - (-3.0f)) < 0.05f, "output gain restored (" + juce::String (out, 2) + " dB)");
        check (restored.getCurrentPresetName() == "BRIGHT VOCAL", "preset index restored");
    }

    // ---------------------------------------------------------------------
    std::printf ("\n6) Reverb tail and denormal safety\n");
    {
        GridVoxProcessor processor;
        if (auto* p = processor.apvts.getParameter (ParamIDs::reverbMix))
            p->setValueNotifyingHost (1.0f);
        processor.prepareToPlay (48000.0, 256);

        juce::AudioBuffer<float> buffer (2, 256);
        juce::MidiBuffer midi;
        juce::Random rng (5);

        fill (buffer, 5, 48000.0, rng);           // single impulse
        processor.processBlock (buffer, midi);

        float tailPeak = 0.0f;
        for (int b = 0; b < 2000; ++b)            // ~10 s of silence
        {
            buffer.clear();
            processor.processBlock (buffer, midi);
            tailPeak = juce::jmax (tailPeak, analyse (buffer).peak);
        }

        const auto endStats = analyse (buffer);
        check (endStats.allFinite, "reverb tail stays finite");
        check (tailPeak < 1.0f, "reverb tail never clips (peak " + juce::String (tailPeak, 3) + ")");
        check (endStats.rms < 0.01, "reverb decays to silence (rms " + juce::String (endStats.rms, 6) + ")");

        // Bring the mix back down and confirm silence is truly silent.
        if (auto* p = processor.apvts.getParameter (ParamIDs::reverbMix))
            p->setValueNotifyingHost (0.0f);
        for (int b = 0; b < 400; ++b)
        {
            buffer.clear();
            processor.processBlock (buffer, midi);
        }
        check (analyse (buffer).peak < 1.0e-4f, "silence in, silence out");
    }

    // ---------------------------------------------------------------------
    std::printf ("\n7) CPU cost\n");
    {
        GridVoxProcessor processor;
        processor.prepareToPlay (48000.0, 512);
        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;
        juce::Random rng (11);
        fill (buffer, 4, 48000.0, rng);

        const auto start = std::chrono::steady_clock::now();
        constexpr int blocks = 2000;
        for (int b = 0; b < blocks; ++b)
            processor.processBlock (buffer, midi);
        const auto elapsed = std::chrono::duration<double> (std::chrono::steady_clock::now() - start).count();

        const double audioSeconds = (double) blocks * 512.0 / 48000.0;
        const double ratio = elapsed / audioSeconds;
        std::printf ("  %d blocks (%.1f s audio) in %.3f s -> %.2f %% of one core\n",
                     blocks, audioSeconds, elapsed, ratio * 100.0);
        check (ratio < 1.0, "processing faster than real time");
    }

    std::printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL CHECKS PASSED" : "FAILURES DETECTED",
                 failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
