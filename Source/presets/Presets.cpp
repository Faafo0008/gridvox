#include "Presets.h"
#include "params/ParameterIDs.h"

namespace Presets
{
    static const std::vector<Preset>& presets()
    {
        static const std::vector<Preset> list = {
            { "CLEAN TRACKING", {
                { ParamIDs::gate, 20.0f }, { ParamIDs::lowCutFreq, 80.0f }, { ParamIDs::lowCutSlope, 2.0f },
                { ParamIDs::mud, 25.0f }, { ParamIDs::grace, 35.0f },
                { ParamIDs::fast, 15.0f }, { ParamIDs::smooth, 20.0f },
                { ParamIDs::eqGain[0], 0.0f }, { ParamIDs::eqFreq[0], 120.0f },
                { ParamIDs::eqGain[1], 0.0f }, { ParamIDs::eqFreq[1], 300.0f },
                { ParamIDs::eqGain[2], 0.0f }, { ParamIDs::eqFreq[2], 2500.0f },
                { ParamIDs::eqGain[3], 0.0f }, { ParamIDs::eqFreq[3], 12000.0f },
                { ParamIDs::deEss, 30.0f }, { ParamIDs::warmth, 20.0f },
                { ParamIDs::reverbMix, 45.0f }, { ParamIDs::reverbType, 1.0f }, { ParamIDs::reverbTime, 1.0f },
                { ParamIDs::reverbLoCut, 320.0f }, { ParamIDs::reverbHiCut, 8000.0f },
                { ParamIDs::output, 4.5f }
            } },
            { "WARM VOCAL", {
                { ParamIDs::gate, 25.0f }, { ParamIDs::lowCutFreq, 95.0f }, { ParamIDs::lowCutSlope, 1.0f },
                { ParamIDs::mud, 35.0f }, { ParamIDs::grace, 25.0f },
                { ParamIDs::fast, 20.0f }, { ParamIDs::smooth, 30.0f },
                { ParamIDs::eqGain[0], 1.5f }, { ParamIDs::eqFreq[0], 100.0f },
                { ParamIDs::eqGain[1], 1.0f }, { ParamIDs::eqFreq[1], 280.0f },
                { ParamIDs::eqGain[2], -0.5f }, { ParamIDs::eqFreq[2], 2200.0f },
                { ParamIDs::eqGain[3], 0.5f }, { ParamIDs::eqFreq[3], 10000.0f },
                { ParamIDs::deEss, 25.0f }, { ParamIDs::warmth, 45.0f },
                { ParamIDs::reverbMix, 35.0f }, { ParamIDs::reverbType, 1.0f }, { ParamIDs::reverbTime, 2.0f },
                { ParamIDs::reverbLoCut, 280.0f }, { ParamIDs::reverbHiCut, 6500.0f },
                { ParamIDs::output, 3.0f }
            } },
            { "BRIGHT VOCAL", {
                { ParamIDs::gate, 30.0f }, { ParamIDs::lowCutFreq, 110.0f }, { ParamIDs::lowCutSlope, 2.0f },
                { ParamIDs::mud, 20.0f }, { ParamIDs::grace, 55.0f },
                { ParamIDs::fast, 25.0f }, { ParamIDs::smooth, 15.0f },
                { ParamIDs::eqGain[0], -1.0f }, { ParamIDs::eqFreq[0], 90.0f },
                { ParamIDs::eqGain[1], -1.5f }, { ParamIDs::eqFreq[1], 350.0f },
                { ParamIDs::eqGain[2], 1.5f }, { ParamIDs::eqFreq[2], 2800.0f },
                { ParamIDs::eqGain[3], 3.0f }, { ParamIDs::eqFreq[3], 12000.0f },
                { ParamIDs::deEss, 45.0f }, { ParamIDs::warmth, 30.0f },
                { ParamIDs::reverbMix, 40.0f }, { ParamIDs::reverbType, 2.0f }, { ParamIDs::reverbTime, 2.0f },
                { ParamIDs::reverbLoCut, 400.0f }, { ParamIDs::reverbHiCut, 9000.0f },
                { ParamIDs::output, 2.5f }
            } },
            { "SMOOTH VOICE", {
                { ParamIDs::gate, 35.0f }, { ParamIDs::lowCutFreq, 70.0f }, { ParamIDs::lowCutSlope, 0.0f },
                { ParamIDs::mud, 40.0f }, { ParamIDs::grace, 30.0f },
                { ParamIDs::fast, 10.0f }, { ParamIDs::smooth, 55.0f },
                { ParamIDs::eqGain[0], 1.0f }, { ParamIDs::eqFreq[0], 110.0f },
                { ParamIDs::eqGain[1], 0.5f }, { ParamIDs::eqFreq[1], 300.0f },
                { ParamIDs::eqGain[2], -1.0f }, { ParamIDs::eqFreq[2], 2500.0f },
                { ParamIDs::eqGain[3], -1.5f }, { ParamIDs::eqFreq[3], 11000.0f },
                { ParamIDs::deEss, 35.0f }, { ParamIDs::warmth, 35.0f },
                { ParamIDs::reverbMix, 55.0f }, { ParamIDs::reverbType, 1.0f }, { ParamIDs::reverbTime, 3.0f },
                { ParamIDs::reverbLoCut, 300.0f }, { ParamIDs::reverbHiCut, 7000.0f },
                { ParamIDs::output, 3.5f }
            } },
            { "BROADCAST", {
                { ParamIDs::gate, 45.0f }, { ParamIDs::lowCutFreq, 90.0f }, { ParamIDs::lowCutSlope, 2.0f },
                { ParamIDs::mud, 30.0f }, { ParamIDs::grace, 45.0f },
                { ParamIDs::fast, 45.0f }, { ParamIDs::smooth, 25.0f },
                { ParamIDs::eqGain[0], -2.0f }, { ParamIDs::eqFreq[0], 120.0f },
                { ParamIDs::eqGain[1], -1.0f }, { ParamIDs::eqFreq[1], 300.0f },
                { ParamIDs::eqGain[2], 2.0f }, { ParamIDs::eqFreq[2], 3000.0f },
                { ParamIDs::eqGain[3], 1.5f }, { ParamIDs::eqFreq[3], 11000.0f },
                { ParamIDs::deEss, 40.0f }, { ParamIDs::warmth, 25.0f },
                { ParamIDs::reverbMix, 15.0f }, { ParamIDs::reverbType, 0.0f }, { ParamIDs::reverbTime, 1.0f },
                { ParamIDs::reverbLoCut, 350.0f }, { ParamIDs::reverbHiCut, 7500.0f },
                { ParamIDs::output, 2.0f }
            } }
        };
        return list;
    }

    const std::vector<Preset>& all() { return presets(); }

    int indexOf (const juce::String& name)
    {
        const auto& list = presets();
        for (int i = 0; i < (int) list.size(); ++i)
            if (name.equalsIgnoreCase (list[(size_t) i].name))
                return i;
        return -1;
    }

    juce::String nameOf (int index)
    {
        const auto& list = presets();
        if (list.empty()) return {};
        return list[(size_t) juce::jlimit (0, (int) list.size() - 1, index)].name;
    }

    int defaultIndex() { return 0; }

    void apply (juce::AudioProcessorValueTreeState& apvts, int index)
    {
        const auto& list = presets();
        if (list.empty()) return;

        const auto& preset = list[(size_t) juce::jlimit (0, (int) list.size() - 1, index)];

        for (const auto& kv : preset.values)
        {
            if (auto* param = apvts.getParameter (kv.first))
            {
                const auto normalised = param->convertTo0to1 (kv.second);
                param->setValueNotifyingHost (normalised);
            }
        }

        apvts.state.setProperty ("presetIndex", index, nullptr);
        apvts.state.setProperty ("presetName", juce::String (preset.name), nullptr);
    }
}
