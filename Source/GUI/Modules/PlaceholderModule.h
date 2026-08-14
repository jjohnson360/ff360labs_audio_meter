#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Common/MeterModule.h"

class PlaceholderModule : public MeterModule
{
public:
    PlaceholderModule(const juce::String& name, MeterModuleType type);
    ~PlaceholderModule() override = default;

    void paintModule(juce::Graphics& g) override;
    void resizedModule() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaceholderModule)
};
