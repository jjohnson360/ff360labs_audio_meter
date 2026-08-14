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

    // Phase 10.5: FFT resolution selector in module header
    juce::ComboBox fftResolutionCombo;

    float getLogX(float index, float numBins, float width);
    void drawSpectrum(juce::Graphics& g,
                      const std::vector<float>& magnitudes,
                      juce::Rectangle<float> plotArea,
                      float minDb, float rangeDb,
                      juce::Colour lineColour,
                      bool drawFill);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzerModule)
};

