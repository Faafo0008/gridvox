/*
    GRID VOX - standalone preview / screenshot harness.

    Uses exactly the same processor + editor code as the VST3 build.

      GridVoxPreview                          -> opens a window (560 x 499)
      GridVoxPreview --render=out.png         -> renders the UI offscreen and exits
      GridVoxPreview --preset="WARM VOCAL"    -> starts on the given preset
      GridVoxPreview --scale=1.5              -> window scale (window mode only)
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ui/EditorContent.h"

#include <cstdio>

namespace
{
    void log (const juce::String& message)
    {
        std::fprintf (stderr, "[gv] %s\n", message.toRawUTF8());
        std::fflush (stderr);
    }

    /** Renders the interface offline at its native size - used for visual checks. */
    bool renderOffscreen (const juce::String& path, const juce::String& presetName, float scale)
    {
        GridVoxProcessor processor;
        processor.prepareToPlay (48000.0, 512);

        if (presetName.isNotEmpty())
        {
            const int index = Presets::indexOf (presetName);
            if (index >= 0)
                processor.setCurrentPreset (index);
        }

        const int w = juce::roundToInt (560.0f * scale);
        const int h = juce::roundToInt (499.0f * scale);

        EditorContent content (processor);
        content.setSize (560, 499);
        content.resized();
        content.refreshValues();

        if (scale == 1.0f)
        {
            content.setBounds (0, 0, 560, 499);
        }
        else
        {
            content.setBounds (0, 0, 560, 499);
            content.setTransform (juce::AffineTransform::scale (scale));
        }

        // Let the meter widgets show their idle state before grabbing the frame.
        content.refresh (processor.readUiStatus());

        juce::Image image (juce::Image::ARGB, w, h, true);
        {
            juce::Graphics g (image);
            g.fillAll (juce::Colour (0xff020507));
            if (scale == 1.0f)
                content.paintEntireComponent (g, true);
            else
                content.paint (g);  // the transform is applied by the component itself
        }

        if (scale != 1.0f)
        {
            // Children are painted through the component transform in normal use;
            // for the scaled offline render we paint the whole tree at 1x first.
            juce::Image base (juce::Image::ARGB, 560, 499, true);
            {
                juce::Graphics g (base);
                g.fillAll (juce::Colour (0xff020507));
                content.setTransform ({});
                content.paintEntireComponent (g, true);
            }
            image = base.rescaled (w, h, juce::Graphics::highResamplingQuality);
        }

        juce::File out (path);
        if (! juce::File::isAbsolutePath (path))
            out = juce::File::getCurrentWorkingDirectory().getChildFile (path);

        out.getParentDirectory().createDirectory();
        out.deleteFile();

        juce::FileOutputStream stream (out);
        if (! stream.openedOk())
        {
            log ("could not open " + out.getFullPathName());
            return false;
        }

        juce::PNGImageFormat png;
        if (! png.writeImageToStream (image, stream))
        {
            log ("png write failed");
            return false;
        }

        log ("rendered " + out.getFullPathName() + " " + juce::String (image.getWidth())
             + "x" + juce::String (image.getHeight()));
        return true;
    }
}

//==============================================================================
class PreviewWindow final : public juce::DocumentWindow
{
public:
    explicit PreviewWindow (const juce::String& name)
        : DocumentWindow (name, juce::Colour (0xff020507), DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar (false);
        setResizable (true, true);
        auto* editor = new GridVoxEditor (processor);
        setContentOwned (editor, true);
        setTitleBarHeight (0);
        setContentComponentSize (560, 499);
        setResizeLimits (420, 374, 1680, 1497);
        centreWithSize (getWidth(), getHeight());
        setVisible (true);
    }

    void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }

    GridVoxProcessor processor;
};

//==============================================================================
class PreviewApp final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return "GRID VOX Preview"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override          { return true; }

    void initialise (const juce::String& commandLine) override
    {
        juce::ArgumentList args ("GridVoxPreview", commandLine);

        const auto renderPath = args.getValueForOption ("--render");
        const auto presetName = args.getValueForOption ("--preset");
        const auto scaleText  = args.getValueForOption ("--scale");

        if (renderPath.isNotEmpty())
        {
            const float scale = scaleText.isNotEmpty() ? juce::jmax (0.25f, scaleText.getFloatValue()) : 1.0f;
            renderOffscreen (renderPath, presetName, scale);
            quit();
            return;
        }

        shotPath = args.getValueForOption ("--shot");
        window = std::make_unique<PreviewWindow> ("GRID VOX");

        if (presetName.isNotEmpty())
        {
            const int index = Presets::indexOf (presetName);
            if (index >= 0)
                window->processor.setCurrentPreset (index);
        }

        if (scaleText.isNotEmpty())
        {
            const float scale = scaleText.getFloatValue();
            if (scale > 0.1f)
                window->setContentComponentSize (juce::roundToInt (560.0f * scale),
                                                 juce::roundToInt (499.0f * scale));
        }

        if (shotPath.isNotEmpty())
            juce::Timer::callAfterDelay (900, [this] { takeShot(); });
    }

    /** Captures the live editor window - exercises the real editor code path. */
    void takeShot()
    {
        if (window == nullptr)
            return;

        if (auto* editor = dynamic_cast<GridVoxEditor*> (window->getContentComponent()))
        {
            const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
            juce::File out = juce::File::isAbsolutePath (shotPath)
                                 ? juce::File (shotPath)
                                 : juce::File::getCurrentWorkingDirectory().getChildFile (shotPath);
            out.deleteFile();
            juce::FileOutputStream stream (out);
            if (stream.openedOk())
            {
                juce::PNGImageFormat png;
                png.writeImageToStream (image, stream);
                log ("window shot " + out.getFullPathName() + " "
                     + juce::String (image.getWidth()) + "x" + juce::String (image.getHeight()));
            }
        }

        quit();
    }

    void shutdown() override { window = nullptr; }
    void systemRequestedQuit() override { quit(); }
    void anotherInstanceStarted (const juce::String&) override {}

private:
    std::unique_ptr<PreviewWindow> window;
    juce::String shotPath;
};

START_JUCE_APPLICATION (PreviewApp)
