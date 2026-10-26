#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/VocalChain.h"
#include "presets/Presets.h"
#include "UiStatus.h"

//==============================================================================
class GridVoxProcessor final : public juce::AudioProcessor
{
public:
    GridVoxProcessor();
    ~GridVoxProcessor() override;

    //== AudioProcessor ========================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "GRID VOX"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return juce::jmax (1, (int) Presets::all().size()); }
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //== Presets ===============================================================
    int getCurrentPreset() const noexcept { return currentPreset; }
    juce::String getCurrentPresetName() const;
    void setCurrentPreset (int index);
    void nextPreset();
    void previousPreset();

    //== Editor helpers ========================================================
    UiStatus readUiStatus() const;
    double getHostBpm() const noexcept { return hostBpm.load (std::memory_order_relaxed); }
    juce::String getTempoDisplay() const;
    const gv::ChainStatus& getChainStatus() const noexcept { return chain.getStatus(); }

    juce::AudioProcessorValueTreeState apvts;

private:
    gv::VocalChain chain;
    gv::ChainParameters cachedParams;
    int currentPreset = 0;

    std::atomic<double> hostBpm { 130.0 };
    std::atomic<float> uiInputPeak { 0.0f }, uiOutputPeak { 0.0f };
    std::atomic<float> uiFastRed { 0.0f }, uiSmoothRed { 0.0f }, uiLimiterRed { 0.0f };
    std::atomic<bool> uiGate { false }, uiMud { false }, uiDeEss { false }, uiLimiting { false };

    void pullParameters();
    void updateParameterCache();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GridVoxProcessor)
};
