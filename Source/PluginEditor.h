#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "ui/EditorContent.h"

class GridVoxProcessor;

//==============================================================================
/** Host window: keeps the 560 x 499 artwork intact and scales it as a whole. */
class GridVoxEditor final : public juce::AudioProcessorEditor,
                            private juce::Timer
{
public:
    explicit GridVoxEditor (GridVoxProcessor&);
    ~GridVoxEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int baseWidth  = 560;
    static constexpr int baseHeight = 499;

private:
    void timerCallback() override;
    bool parametersChanged();

    GridVoxProcessor& processor;
    EditorContent content;
    std::vector<float> lastParameterValues;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GridVoxEditor)
};
