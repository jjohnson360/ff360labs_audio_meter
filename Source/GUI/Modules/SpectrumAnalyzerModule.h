#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Common/MeterModule.h"
#include "../../Core/AudioFifo.h"
#include "../../DSP/SpectrumDSP.h"

class SpectrumAnalyzerModule : public MeterModule, public juce::Timer
{
public:
    SpectrumAnalyzerModule(AudioFifo<SpectrumData>& fifoToUse, double sampleRate);
    ~SpectrumAnalyzerModule() override;

    void paintModule(juce::Graphics& g) override;
    void resizedModule() override;
    
    void timerCallback() override;

private:
    AudioFifo<SpectrumData>& meterFifo;
    SpectrumData currentData;
    double currentSampleRate;

    float getLogX(float index, float numBins, float width);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzerModule)
};
