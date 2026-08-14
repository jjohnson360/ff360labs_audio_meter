#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

enum class MeterModuleType
{
    PeakRms,
    VU,
    LUFS,
    PhaseScope,
    Spectrum,
    Histogram,
    Unknown
};

class MeterModule : public juce::Component
{
public:
    MeterModule(const juce::String& name, MeterModuleType type);
    ~MeterModule() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Derived classes should override this to draw their specific DSP content
    virtual void paintModule(juce::Graphics& g) = 0;
    
    // Derived classes should override this for their internal component layout
    virtual void resizedModule() = 0;

    const juce::String& getModuleName() const { return moduleName; }
    MeterModuleType getModuleType() const { return moduleType; }
    juce::Rectangle<int> getModuleBounds() const;

    virtual int getTargetRefreshRateHz() const { return 30; }
    virtual void setThrottleMultiplier(float multiplier) { juce::ignoreUnused(multiplier); }

    std::function<void(MeterModule*)> onClose;
    std::function<void(MeterModule*)> onMaximize;
    std::function<void(MeterModule*)> onDetach;

private:
    juce::String moduleName;
    MeterModuleType moduleType;
    juce::TextButton closeButton{ "X" };
    juce::TextButton maximizeButton{ "[ ]" };
    juce::TextButton detachButton{ "[^]" };
    
    static constexpr int headerHeight = 24;
    static constexpr int padding = 4;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeterModule)
};
