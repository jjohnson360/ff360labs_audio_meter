#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Common/MeterModule.h"

class FloatingModuleWindow : public juce::DocumentWindow
{
public:
    FloatingModuleWindow (MeterModule* moduleToHost, std::function<void(MeterModule*)> onWindowClosed)
        : DocumentWindow (moduleToHost != nullptr ? moduleToHost->getModuleName() : "Detached Module",
                          juce::Colour (0xFF0A0A0B),
                          DocumentWindow::allButtons),
          hostedModule (moduleToHost),
          onCloseCallback (onWindowClosed)
    {
        setUsingNativeTitleBar (false);
        if (hostedModule != nullptr)
        {
            setContentNonOwned (hostedModule, true);
        }
        setResizable (true, true);
        setResizeLimits (280, 200, 1920, 1080);
        centreWithSize (460, 320);
        setAlwaysOnTop (true);
        setVisible (true);
    }

    ~FloatingModuleWindow() override
    {
        clearContentComponent();
    }

    void closeButtonPressed() override
    {
        if (hostedModule != nullptr)
        {
            auto* m = hostedModule;
            hostedModule = nullptr;
            clearContentComponent();
            if (onCloseCallback)
                onCloseCallback (m);
        }
    }

    MeterModule* getHostedModule() const { return hostedModule; }

private:
    MeterModule* hostedModule = nullptr;
    std::function<void(MeterModule*)> onCloseCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FloatingModuleWindow)
};
