#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Common/MeterModule.h"
#include "../../Core/AudioFifo.h"
#include "../../DSP/SpectrumDSP.h"

class SpectrumAnalyzerModule : public MeterModule, public juce::Timer
{
public:
    // The chosen FFT resolution is stored with the session (apvts.state "fftOrder")
    SpectrumAnalyzerModule(AudioFifo<SpectrumData, 8>& fifoToUse, SpectrumDSP& dspToUse, juce::AudioProcessorValueTreeState& apvts);
    ~SpectrumAnalyzerModule() override;

    void paintModule(juce::Graphics& g) override;
    void resizedModule() override;
    
    void timerCallback() override;

private:
    AudioFifo<SpectrumData, 8>& meterFifo;
    SpectrumDSP& spectrumDSP;
    juce::AudioProcessorValueTreeState& apvts;
    SpectrumData currentData;
    double currentSampleRate = 48000.0;

    // Phase 10.5: FFT resolution selector in module header
    juce::ComboBox fftResolutionCombo;

    float getLogX(float index, float numBins, float width);
    void drawSpectrum(juce::Graphics& g,
                      const float* magnitudes, int numBins,
                      juce::Rectangle<float> plotArea,
                      float minDb, float rangeDb,
                      juce::Colour lineColour,
                      bool drawFill);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzerModule)
};

