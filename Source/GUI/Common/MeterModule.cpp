#include "MeterModule.h"
#include "../../Core/Constants.h"
#include "../LookAndFeel/FF360LabsLookAndFeel.h"

MeterModule::MeterModule(const juce::String& name, MeterModuleType type)
    : moduleName(name), moduleType(type)
{
    addAndMakeVisible(detachButton);
    addAndMakeVisible(maximizeButton);
    addAndMakeVisible(closeButton);
    
    closeButton.onClick = [this] { if (onClose) onClose(this); };
    maximizeButton.onClick = [this] { if (onMaximize) onMaximize(this); };
    detachButton.onClick = [this] { if (onDetach) onDetach(this); };
}

void MeterModule::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Glass Card Treatment (Frosted Glass, Sheen, Hairline Border)
    FF360LabsLookAndFeel::drawGlassPanel(g, bounds, 8.0f);

    // Slim Module Header
    auto headerRect = bounds.removeFromTop((float)headerHeight);
    
    // Hairline Separator under Header
    g.setColour(ff360_labs::HairlineBorder);
    g.drawHorizontalLine((int)headerRect.getBottom(), headerRect.getX() + 6.0f, headerRect.getRight() - 6.0f);

    // Header Text
    g.setColour(ff360_labs::TextOffWhite);
    g.setFont(FF360LabsLookAndFeel::getCustomFont(11.0f, juce::Font::bold));
    g.drawText(moduleName.toUpperCase(),
               headerRect.toNearestInt().withTrimmedLeft(padding * 2 + 2),
               juce::Justification::centredLeft,
               true);

    // Give derived class a chance to paint in the remaining area
    juce::Graphics::ScopedSaveState state(g);
    g.reduceClipRegion(getModuleBounds());
    paintModule(g);
}

void MeterModule::resized()
{
    auto headerRect = getLocalBounds().removeFromTop(headerHeight);
    
    closeButton.setBounds(headerRect.removeFromRight(headerHeight).reduced(4));
    maximizeButton.setBounds(headerRect.removeFromRight(headerHeight).reduced(4));
    detachButton.setBounds(headerRect.removeFromRight(headerHeight).reduced(4));
    
    // Delegate to derived class
    resizedModule();
}

juce::Rectangle<int> MeterModule::getModuleBounds() const
{
    return getLocalBounds().withTrimmedTop(headerHeight).reduced(padding);
}
