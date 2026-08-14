#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Common/MeterModule.h"
#include "../Common/EasingUtils.h"
#include "../../Core/AudioFifo.h"
#include "../../DSP/VuDSP.h"

class VuMeterModule : public MeterModule, public juce::Timer
{
public:
    VuMeterModule(AudioFifo<VuMeterData>& fifoToUse);
    ~VuMeterModule() override;

    void paintModule(juce::Graphics& g) override;
    void resizedModule() override;
    
    void timerCallback() override;

private:
    AudioFifo<VuMeterData>& meterFifo;
    VuMeterData currentData;

    ff360_labs::SpringDamper needleL { -20.0f, 260.0f, 28.0f };
    ff360_labs::SpringDamper needleR { -20.0f, 260.0f, 28.0f };

    void drawVuArcGauge (juce::Graphics& g, juce::Rectangle<float> bounds, float smoothedVu, const juce::String& channelLabel);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VuMeterModule)
};
