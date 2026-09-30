#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Common/MeterModule.h"

class FloatingModuleWindow : public juce::DocumentWindow
{
public:
    FloatingModuleWindow (MeterModule* moduleToHost, std::function<void(MeterModule*)> onWindowClosed,
                          juce::LookAndFeel* lookAndFeel, float uiZoom)
        : DocumentWindow (moduleToHost != nullptr ? moduleToHost->getModuleName() : "Detached Module",
                          juce::Colour (0xFF0A0A0B),
                          DocumentWindow::allButtons),
          hostedModule (moduleToHost),
          onCloseCallback (onWindowClosed)
    {
        // Top-level windows don't inherit the editor's look and feel; the dashboard resets
        // this before the editor (and its look and feel) goes away
        setLookAndFeel (lookAndFeel);
        setUsingNativeTitleBar (false);

        holder.setZoom (uiZoom);
        if (hostedModule != nullptr)
            holder.setModule (hostedModule);
        setContentNonOwned (&holder, false);

        setResizable (true, true);
        setResizeLimits (juce::roundToInt (280 * uiZoom), juce::roundToInt (200 * uiZoom), 3840, 2160);
        centreWithSize (juce::roundToInt (460 * uiZoom), juce::roundToInt (320 * uiZoom));
        setAlwaysOnTop (true);
        setVisible (true);
    }

    ~FloatingModuleWindow() override
    {
        holder.setModule (nullptr);
        clearContentComponent();
        setLookAndFeel (nullptr);
    }

    void closeButtonPressed() override
    {
        if (hostedModule != nullptr)
        {
            auto* m = hostedModule;
            hostedModule = nullptr;
            holder.setModule (nullptr);
            clearContentComponent();
            if (onCloseCallback)
                onCloseCallback (m);
        }
    }

    void setZoom (float uiZoom) { holder.setZoom (uiZoom); }

    MeterModule* getHostedModule() const { return hostedModule; }

private:
    // Lays the module out at (size / zoom) and scales it, like the editor's canvas
    struct ScaledHolder : public juce::Component
    {
        void setModule (MeterModule* m)
        {
            if (module != nullptr)
            {
                module->setTransform ({});
                removeChildComponent (module);
            }

            module = m;
            if (module != nullptr)
            {
                addAndMakeVisible (module);
                resized();
            }
        }

        void setZoom (float z) { zoom = z; resized(); }

        void resized() override
        {
            if (module == nullptr)
                return;

            module->setTransform ({});
            module->setBounds (0, 0, juce::roundToInt ((float) getWidth() / zoom), juce::roundToInt ((float) getHeight() / zoom));
            module->setTransform (juce::AffineTransform::scale (zoom));
        }

        juce::Component::SafePointer<MeterModule> module;
        float zoom = 1.0f;
    };

    MeterModule* hostedModule = nullptr;
    ScaledHolder holder;
    std::function<void(MeterModule*)> onCloseCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FloatingModuleWindow)
};
