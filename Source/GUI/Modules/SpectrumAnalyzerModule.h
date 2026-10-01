#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Common/MeterModule.h"
#include "../../Core/AudioFifo.h"
#include "../../DSP/SpectrumDSP.h"
#include <array>

class SpectrumAnalyzerModule : public MeterModule, public juce::Timer
{
public:
    // The chosen FFT resolution is stored with the session (apvts.state "fftOrder")
    SpectrumAnalyzerModule(AudioFifo<SpectrumData, 8>& fifoToUse, SpectrumDSP& dspToUse, juce::AudioProcessorValueTreeState& apvts);
    ~SpectrumAnalyzerModule() override;

    void paintModule(juce::Graphics& g) override;
    void resizedModule() override;
    
    void timerCallback() override;

    // Left-channel levels at the log-spaced display points (20 Hz to Nyquist), for tests
    const float* getDisplayLevels() const { return displayL.data(); }
    int getNumDisplayPoints() const { return mappedBins > 1 ? NumDisplayPoints : 0; }

private:
    AudioFifo<SpectrumData, 8>& meterFifo;
    SpectrumDSP& spectrumDSP;
    juce::AudioProcessorValueTreeState& apvts;
    SpectrumData currentData;
    double currentSampleRate = 48000.0;

    // Phase 10.5: FFT resolution selector in module header
    juce::ComboBox fftResolutionCombo;

    // Maps the linear-frequency FFT bins onto log-spaced display points (as Niche360's
    // DisplayMap): Catmull-Rom interpolation where bins are sparse (low end), triangular
    // power averaging where they are dense (top end). Rebuilt when the bin count or rate changes.
    static constexpr int NumDisplayPoints = 256;
    static constexpr double SmoothingOctaves = 1.0 / 6.0; // full width of the averaging band

    struct DisplayPoint
    {
        float centre = 1.0f;          // fractional bin position of the display frequency
        float lo = 1.0f, hi = 1.0f;   // averaging band edges, in bins
        int k0 = 1, k1 = 1;           // first / last bin inside the band
        float invRise = 0.0f, invFall = 0.0f, invTotalW = 0.0f;
        float bandBlend = 0.0f;       // 0 = interpolate, 1 = band average

        float weight(int k) const
        {
            const float x = (float) k;
            return x < centre ? (x - lo) * invRise : (hi - x) * invFall;
        }
    };

    std::array<DisplayPoint, NumDisplayPoints> displayPoints {};
    std::array<float, NumDisplayPoints> displayL {}, displayR {};
    int mappedBins = 0;
    double mappedSampleRate = 0.0;

    void rebuildDisplayMap(int numBins, double sampleRate);
    void mapToDisplay(const float* binsDb, int numBins, float* outDb) const;
    void drawSpectrum(juce::Graphics& g,
                      const float* displayDb,
                      juce::Rectangle<float> plotArea,
                      float minDb, float rangeDb,
                      juce::Colour lineColour,
                      bool drawFill);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzerModule)
};

