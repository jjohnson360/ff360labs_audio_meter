#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Common/MeterModule.h"
#include "../../Core/AudioFifo.h"
#include "../../DSP/PeakRmsDSP.h"

class PeakRmsMeterModule : public MeterModule, public juce::Timer
{
public:
    PeakRmsMeterModule(AudioFifo<MeterData>& fifoToUse);
    ~PeakRmsMeterModule() override;

    void paintModule(juce::Graphics& g) override;
    void resizedModule() override;
    
    void timerCallback() override;

private:
    AudioFifo<MeterData>& meterFifo;
    PeakRmsDSP dsp; // used for ballistics/smoothing logic only on UI thread
    
    MeterData currentSmoothedData;

    void drawMeterLane(juce::Graphics& g, juce::Rectangle<float> bounds, float peakDb, float rmsDb);
    float jmapDbToHeight(float db, float height);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PeakRmsMeterModule)
};
