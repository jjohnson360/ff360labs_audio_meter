#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Common/MeterModule.h"
#include "../../Core/AudioFifo.h"
#include "../../DSP/PhaseScopeDSP.h"

class PhaseScopeModule : public MeterModule, public juce::Timer
{
public:
    PhaseScopeModule(AudioFifo<PhaseScopeData>& fifoToUse);
    ~PhaseScopeModule() override;

    void paintModule(juce::Graphics& g) override;
    void resizedModule() override;
    
    void timerCallback() override;

private:
    AudioFifo<PhaseScopeData>& meterFifo;
    PhaseScopeData currentData;

    juce::Image scopeImage;

    // Wall-clock timestamp (ms) of the last successful persistence decay, so the
    // fade rate stays constant in real time regardless of how often the OS
    // actually services repaint() (see updateScopeImage()).
    double lastDecayTimeMs = 0.0;

    void updateScopeImage(juce::Rectangle<float> bounds);
    void drawCorrelationMeter(juce::Graphics& g, juce::Rectangle<float> bounds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhaseScopeModule)
};
