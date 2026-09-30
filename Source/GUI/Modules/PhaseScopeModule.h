#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include "../Common/MeterModule.h"
#include "../../Core/AudioFifo.h"
#include "../../DSP/PhaseScopeDSP.h"

class PhaseScopeModule : public MeterModule, public juce::Timer
{
public:
    PhaseScopeModule(AudioFifo<PhaseScopeData, 128>& fifoToUse);
    ~PhaseScopeModule() override;

    void paintModule(juce::Graphics& g) override;
    void resizedModule() override;
    
    void timerCallback() override;

private:
    AudioFifo<PhaseScopeData, 128>& meterFifo;

    // UI-side accumulation of the frames drained since the last paint
    struct ScopeState
    {
        float correlation = 0.0f;
        std::vector<PhaseScopePoint> samplePairs;
    } currentData;

    juce::Image scopeImage;

    // Wall-clock timestamp (ms) of the last successful persistence decay, so the
    // fade rate stays constant in real time regardless of how often the OS
    // actually services repaint() (see updateScopeImage()).
    double lastDecayTimeMs = 0.0;

    void updateScopeImage(juce::Rectangle<float> bounds, float pixelScale);
    void drawCorrelationMeter(juce::Graphics& g, juce::Rectangle<float> bounds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhaseScopeModule)
};
