#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "params/Parameters.h"
#include "params/ParameterIDs.h"

using namespace juce;

//==============================================================================
GridVoxProcessor::GridVoxProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "GRIDVOX", Params::createLayout())
{
    // Start on the reference patch.
    currentPreset = Presets::defaultIndex();
    Presets::apply (apvts, currentPreset);
    setLatencySamples (gv::VocalChain::getLatencySamples());
}

GridVoxProcessor::~GridVoxProcessor() = default;

//==============================================================================
void GridVoxProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    chain.prepare (sampleRate, samplesPerBlock);
    updateParameterCache();
    chain.setParameters (cachedParams);
    chain.reset();
    setLatencySamples (gv::VocalChain::getLatencySamples());
}

void GridVoxProcessor::releaseResources()
{
    chain.reset();
}

//==============================================================================
void GridVoxProcessor::updateParameterCache()
{
    auto get = [this] (const char* id) -> float
    {
        if (auto* p = apvts.getRawParameterValue (id))
            return p->load();
        return 0.0f;
    };

    cachedParams.gate        = get (ParamIDs::gate);
    cachedParams.lowCutHz    = get (ParamIDs::lowCutFreq);
    cachedParams.lowCutSlope = 12 * (1 + roundToInt (get (ParamIDs::lowCutSlope)));
    cachedParams.mud         = get (ParamIDs::mud);
    cachedParams.grace       = get (ParamIDs::grace);
    cachedParams.deEss       = get (ParamIDs::deEss);
    cachedParams.warmth      = get (ParamIDs::warmth);
    cachedParams.fast        = get (ParamIDs::fast);
    cachedParams.smooth      = get (ParamIDs::smooth);

    for (int i = 0; i < Params::numEqBands; ++i)
    {
        cachedParams.eqGain[i] = get (ParamIDs::eqGain[i]);
        cachedParams.eqFreq[i] = get (ParamIDs::eqFreq[i]);
    }

    cachedParams.reverbMix   = get (ParamIDs::reverbMix);
    cachedParams.reverbType  = roundToInt (get (ParamIDs::reverbType));
    cachedParams.reverbLoCut = get (ParamIDs::reverbLoCut);
    cachedParams.reverbHiCut = get (ParamIDs::reverbHiCut);
    cachedParams.reverbDecay = (float) Params::reverbDecaySeconds (roundToInt (get (ParamIDs::reverbTime)),
                                                                  hostBpm.load (std::memory_order_relaxed));

    cachedParams.outputDb = get (ParamIDs::output);
    cachedParams.bypass   = get (ParamIDs::bypass) > 0.5f;
}

void GridVoxProcessor::pullParameters()
{
    updateParameterCache();
    chain.setParameters (cachedParams);
}

//==============================================================================
void GridVoxProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    midi.clear();

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numSamples <= 0)
        return;

    // Host tempo (used by the tempo synced reverb decay display).
    if (auto* playHead = getPlayHead())
    {
        if (const auto pos = playHead->getPosition())
        {
            if (const auto bpm = pos->getBpm())
                hostBpm.store (*bpm, std::memory_order_relaxed);
        }
    }

    pullParameters();

    // Mono-in/stereo-out and other layouts are handled by working on the first
    // two channels (duplicating mono when needed).
    float* left = buffer.getWritePointer (0);
    float* right = numChannels > 1 ? buffer.getWritePointer (1) : left;

    chain.process (left, right, numSamples);

    const auto& s = chain.getStatus();
    uiInputPeak.store (s.inputPeak);
    uiOutputPeak.store (s.outputPeak);
    uiFastRed.store (s.fastReductionDb);
    uiSmoothRed.store (s.smoothReductionDb);
    uiLimiterRed.store (s.limiterReductionDb);
    uiGate.store (s.gateActive);
    uiMud.store (s.mudActive);
    uiDeEss.store (s.deEssActive);
    uiLimiting.store (s.limiterActive);
}

//==============================================================================
UiStatus GridVoxProcessor::readUiStatus() const
{
    UiStatus s;
    s.inputPeak  = uiInputPeak.load();
    s.outputPeak = uiOutputPeak.load();
    s.fastReductionDb = uiFastRed.load();
    s.smoothReductionDb = uiSmoothRed.load();
    s.limiterReductionDb = uiLimiterRed.load();
    s.gateActive = uiGate.load();
    s.mudActive = uiMud.load();
    s.deEssActive = uiDeEss.load();
    s.limiterActive = uiLimiting.load();
    return s;
}

//==============================================================================
String GridVoxProcessor::getCurrentPresetName() const
{
    return Presets::nameOf (currentPreset);
}

void GridVoxProcessor::setCurrentPreset (int index)
{
    const int count = (int) Presets::all().size();
    if (count == 0) return;

    currentPreset = jlimit (0, count - 1, index);
    Presets::apply (apvts, currentPreset);
    updateParameterCache();
    chain.setParameters (cachedParams);

    if (auto* ed = getActiveEditor())
        ed->repaint();
}

void GridVoxProcessor::nextPreset()
{
    const int count = (int) Presets::all().size();
    setCurrentPreset ((currentPreset + 1) % jmax (1, count));
}

void GridVoxProcessor::previousPreset()
{
    const int count = (int) Presets::all().size();
    setCurrentPreset ((currentPreset - 1 + count) % jmax (1, count));
}

void GridVoxProcessor::setCurrentProgram (int index) { setCurrentPreset (index); }
const String GridVoxProcessor::getProgramName (int index) { return Presets::nameOf (index); }


String GridVoxProcessor::getTempoDisplay() const
{
    const auto bpm = hostBpm.load (std::memory_order_relaxed);
    const double halfNoteSeconds = 2.0 * 60.0 / jmax (20.0, bpm);
    return "1/2 = " + String (halfNoteSeconds, 2) + " S @ " + String (roundToInt (bpm)) + " BPM";
}

//==============================================================================
void GridVoxProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("presetIndex", currentPreset, nullptr);
    state.setProperty ("presetName", getCurrentPresetName(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void GridVoxProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = ValueTree::fromXml (*xml);
            apvts.replaceState (tree);

            const int index = (int) tree.getProperty ("presetIndex", Presets::defaultIndex());
            const int count = (int) Presets::all().size();
            currentPreset = jlimit (0, jmax (0, count - 1), index);

            updateParameterCache();
            chain.setParameters (cachedParams);

            if (auto* ed = getActiveEditor())
                ed->repaint();
        }
    }
}

//==============================================================================
AudioProcessorEditor* GridVoxProcessor::createEditor()
{
    return new GridVoxEditor (*this);
}

//==============================================================================
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GridVoxProcessor();
}
