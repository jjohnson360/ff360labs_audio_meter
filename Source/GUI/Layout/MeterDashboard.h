#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Common/MeterModule.h"

enum class LayoutMode
{
    Grid,
    Maximized
};

#include "FloatingModuleWindow.h"

class MeterDashboard : public juce::Component
{
public:
    MeterDashboard();
    ~MeterDashboard() override;

    void resized() override;

    void addModule(MeterModule* moduleToAdd);
    void removeModule(MeterModule* moduleToRemove);
    void clearAllModules();
    void detachModule(MeterModule* moduleToDetach);
    void reDockModule(MeterModule* moduleToReDock);
    void setLayoutMode(LayoutMode mode);
    void setFocusedModule(MeterModule* moduleToFocus);
    
    LayoutMode getLayoutMode() const { return currentMode; }
    const juce::Array<MeterModule*>& getModules() const { return modules; }

private:
    juce::Array<MeterModule*> modules;
    juce::OwnedArray<FloatingModuleWindow> floatingWindows;
    LayoutMode currentMode = LayoutMode::Grid;
    MeterModule* focusedModule = nullptr;
    juce::ComponentAnimator animator;

    void updateGridLayout();
    void updateMaximizedLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeterDashboard)
};
