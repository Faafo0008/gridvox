#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

/*  Factory presets. Each preset defines every parameter so that switching is
    fully deterministic (no leftovers from the previous patch).
*/
namespace Presets
{
    struct Preset
    {
        const char* name;
        std::vector<std::pair<const char*, float>> values;
    };

    const std::vector<Preset>& all();
    int indexOf (const juce::String& name);
    juce::String nameOf (int index);

    /** Applies the preset to the parameter tree (message thread only). */
    void apply (juce::AudioProcessorValueTreeState& apvts, int index);

    /** Index of CLEAN TRACKING, the default patch shown in the reference. */
    int defaultIndex();
}
