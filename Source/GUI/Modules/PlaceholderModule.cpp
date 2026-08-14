#include "PlaceholderModule.h"
#include "../../Core/Constants.h"

PlaceholderModule::PlaceholderModule(const juce::String& name, MeterModuleType type)
    : MeterModule(name, type)
{
}

void PlaceholderModule::paintModule(juce::Graphics& g)
{
    auto bounds = getModuleBounds();
    
    g.setColour(ff360_labs::ContainerOutline.darker(0.8f));
    g.fillRect(bounds);
    
    g.setColour(ff360_labs::TextMuted);
    g.setFont(14.0f);
    g.drawFittedText("LUFS / Phase Scope Target Slot", bounds, juce::Justification::centred, 2);
}

void PlaceholderModule::resizedModule()
{
}
