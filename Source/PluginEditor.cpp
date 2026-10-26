#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
GridVoxEditor::GridVoxEditor (GridVoxProcessor& p)
    : AudioProcessorEditor (&p), processor (p), content (p)
{
    setOpaque (true);
    addAndMakeVisible (content);
    setResizable (true, true);
    setResizeLimits (420, 374, 1680, 1497);
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio ((double) baseWidth / (double) baseHeight);

    setSize (baseWidth, baseHeight);

    content.refreshValues();
    startTimerHz (30);
}

GridVoxEditor::~GridVoxEditor()
{
    stopTimer();
}

//==============================================================================
void GridVoxEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff020507));
}

void GridVoxEditor::resized()
{
    // Uniform scale, never stretches the circular knobs.
    const auto scale = juce::jmin ((float) getWidth()  / (float) baseWidth,
                                   (float) getHeight() / (float) baseHeight);

    content.setTransform (juce::AffineTransform::scale (scale));
    const int w = juce::roundToInt ((float) baseWidth  * scale);
    const int h = juce::roundToInt ((float) baseHeight * scale);
    content.setBounds (0, 0, w, h);
    content.setTopLeftPosition ((getWidth() - w) / 2, (getHeight() - h) / 2);
}

//==============================================================================
void GridVoxEditor::timerCallback()
{
    content.refresh (processor.readUiStatus());

    if (parametersChanged())
        content.refreshValues();
}

bool GridVoxEditor::parametersChanged()
{
    const auto& params = processor.getParameters();
    bool changed = false;

    if (lastParameterValues.size() != (size_t) params.size())
    {
        lastParameterValues.assign ((size_t) params.size(), -1.0f);
        changed = true;
    }

    for (int i = 0; i < params.size(); ++i)
    {
        const auto v = params[i]->getValue();
        if (std::abs (v - lastParameterValues[(size_t) i]) > 1.0e-6f)
        {
            lastParameterValues[(size_t) i] = v;
            changed = true;
        }
    }

    return changed;
}
