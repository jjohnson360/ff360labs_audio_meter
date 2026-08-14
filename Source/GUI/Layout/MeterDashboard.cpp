#include "MeterDashboard.h"

MeterDashboard::MeterDashboard()
{
}

MeterDashboard::~MeterDashboard()
{
    floatingWindows.clear();
}

void MeterDashboard::addModule(MeterModule* moduleToAdd)
{
    if (moduleToAdd != nullptr)
    {
        modules.add(moduleToAdd);
        addAndMakeVisible(moduleToAdd);
        
        moduleToAdd->onClose = [this](MeterModule* m) {
            removeModule(m);
        };
        
        moduleToAdd->onMaximize = [this](MeterModule* m) {
            if (currentMode == LayoutMode::Grid)
            {
                setFocusedModule(m);
                setLayoutMode(LayoutMode::Maximized);
            }
            else
            {
                setLayoutMode(LayoutMode::Grid);
            }
        };

        moduleToAdd->onDetach = [this](MeterModule* m) {
            detachModule(m);
        };
        
        resized();
    }
}

void MeterDashboard::detachModule(MeterModule* moduleToDetach)
{
    if (moduleToDetach != nullptr && modules.contains(moduleToDetach))
    {
        modules.removeFirstMatchingValue(moduleToDetach);
        removeChildComponent(moduleToDetach);
        if (focusedModule == moduleToDetach)
            focusedModule = nullptr;

        resized();

        auto* floatingWin = new FloatingModuleWindow(moduleToDetach, [this](MeterModule* m) {
            reDockModule(m);
        });
        floatingWindows.add(floatingWin);
    }
}

void MeterDashboard::reDockModule(MeterModule* moduleToReDock)
{
    if (moduleToReDock != nullptr)
    {
        for (int i = floatingWindows.size() - 1; i >= 0; --i)
        {
            if (floatingWindows[i]->getHostedModule() == moduleToReDock || floatingWindows[i]->getHostedModule() == nullptr)
            {
                floatingWindows.remove(i);
                break;
            }
        }
        addModule(moduleToReDock);
    }
}

void MeterDashboard::removeModule(MeterModule* moduleToRemove)
{
    if (moduleToRemove != nullptr && modules.contains(moduleToRemove))
    {
        modules.removeFirstMatchingValue(moduleToRemove);
        removeChildComponent(moduleToRemove);
        if (focusedModule == moduleToRemove)
            focusedModule = nullptr;
        
        if (currentMode == LayoutMode::Maximized && modules.isEmpty())
        {
            setLayoutMode(LayoutMode::Grid);
        }
        else
        {
            resized();
        }
    }
}

void MeterDashboard::clearAllModules()
{
    floatingWindows.clear();
    for (auto* m : modules)
    {
        removeChildComponent (m);
    }
    modules.clear();
    focusedModule = nullptr;
    resized();
}

void MeterDashboard::setLayoutMode(LayoutMode mode)
{
    if (currentMode != mode)
    {
        currentMode = mode;
        
        if (currentMode == LayoutMode::Maximized && focusedModule == nullptr && !modules.isEmpty())
        {
            focusedModule = modules.getFirst(); // Default fallback
        }
        
        resized();
    }
}

void MeterDashboard::setFocusedModule(MeterModule* moduleToFocus)
{
    if (modules.contains(moduleToFocus))
    {
        focusedModule = moduleToFocus;
        if (currentMode == LayoutMode::Maximized)
            resized();
    }
}

void MeterDashboard::resized()
{
    if (modules.isEmpty())
        return;

    if (currentMode == LayoutMode::Grid)
    {
        updateGridLayout();
    }
    else if (currentMode == LayoutMode::Maximized)
    {
        updateMaximizedLayout();
    }
}

void MeterDashboard::updateGridLayout()
{
    juce::Grid grid;
    grid.rowGap = juce::Grid::Px(4);
    grid.columnGap = juce::Grid::Px(4);
    
    // Auto-flow items
    grid.autoFlow = juce::Grid::AutoFlow::row;
    
    // Define a responsive minimum track size
    using Track = juce::Grid::TrackInfo;
    grid.templateColumns = { Track(juce::Grid::Fr(1)), Track(juce::Grid::Fr(1)) };
    grid.templateRows = { Track(juce::Grid::Fr(1)) };
    grid.autoRows = Track(juce::Grid::Fr(1));

    for (auto* m : modules)
    {
        grid.items.add(juce::GridItem(m));
        
        // Ensure they are visible and animate them into their grid position
        m->setVisible(true);
    }
    
    grid.performLayout(getLocalBounds());
    
    // Override instantaneous placement with animation
    for (auto* m : modules)
    {
        auto targetBounds = m->getBounds();
        
        // If it was collapsed, start it small
        if (m->getWidth() == 0) 
            m->setBounds(targetBounds.withSize(1, 1).withPosition(targetBounds.getCentre()));

        animator.animateComponent(m, targetBounds, 1.0f, 250, false, 1.0, 1.0);
    }
}

void MeterDashboard::updateMaximizedLayout()
{
    auto fullBounds = getLocalBounds();
    
    for (auto* m : modules)
    {
        if (m == focusedModule)
        {
            m->setVisible(true);
            m->toBehind(nullptr); // bring to front
            animator.animateComponent(m, fullBounds, 1.0f, 250, false, 1.0, 1.0);
        }
        else
        {
            // Animate others to a zero-sized rect in the center, fading out
            auto targetBounds = fullBounds.withSize(1, 1).withPosition(fullBounds.getCentre());
            animator.animateComponent(m, targetBounds, 0.0f, 250, false, 1.0, 1.0);
        }
    }
}
