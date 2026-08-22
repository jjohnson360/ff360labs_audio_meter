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

    // Vector-rendered Kebab/Options menu button (Phase 11.2 - immune to font/encoding bugs)
    class KebabMenuButton : public juce::Button
    {
    public:
        KebabMenuButton() : juce::Button("ModuleOptions")
        {
            setTooltip("Module Options");
        }

        void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
        {
            auto bounds = getLocalBounds().toFloat();
            
            if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
            {
                g.setColour(juce::Colour(0xFF17171A).brighter(0.12f).withAlpha(0.85f));
                g.fillRoundedRectangle(bounds, 3.0f);
                g.setColour(juce::Colour(0xFFC9A15A).withAlpha(0.45f));
                g.drawRoundedRectangle(bounds.reduced(0.5f), 3.0f, 1.0f);
            }

            juce::Colour dotCol = shouldDrawButtonAsDown ? juce::Colour(0xFFC9A15A)
                                : (shouldDrawButtonAsHighlighted ? juce::Colour(0xFFEDEAE3) : juce::Colour(0xFF8A8780));
            g.setColour(dotCol);

            float cx = bounds.getCentreX();
            float cy = bounds.getCentreY();
            float dotRadius = 1.25f;
            float spacing = 4.0f;

            g.fillEllipse(cx - dotRadius, cy - spacing - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
            g.fillEllipse(cx - dotRadius, cy - dotRadius,           dotRadius * 2.0f, dotRadius * 2.0f);
            g.fillEllipse(cx - dotRadius, cy + spacing - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
        }
    };

    KebabMenuButton menuButton;
    void showModuleMenu();
    
    static constexpr int headerHeight = 24;
    static constexpr int padding = 4;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeterModule)
};

