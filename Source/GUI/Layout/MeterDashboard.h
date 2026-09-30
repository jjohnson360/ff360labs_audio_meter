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

    // Detached module windows are scaled like the editor
    void setUiZoom(float zoom);

    // Called whenever the set of docked modules or the layout mode changes
    std::function<void()> onLayoutChanged;
    const juce::Array<MeterModule*>& getModules() const { return modules; }

private:
    juce::Array<MeterModule*> modules;
    juce::OwnedArray<FloatingModuleWindow> floatingWindows;
    LayoutMode currentMode = LayoutMode::Grid;
    MeterModule* focusedModule = nullptr;
    float uiZoom = 1.0f;
    juce::ComponentAnimator animator;

    void updateGridLayout();
    void updateMaximizedLayout();
    void notifyLayoutChanged() { if (onLayoutChanged) onLayoutChanged(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeterDashboard)
};
