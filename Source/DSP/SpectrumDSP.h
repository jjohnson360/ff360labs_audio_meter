#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>

struct SpectrumData
{
    std::vector<float> magnitudes;
};

class SpectrumDSP
{
public:
    SpectrumDSP();

    void prepare(double sampleRate, int samplesPerBlock);
    
    // Process block and return smoothed spectrum if a new FFT was calculated, 
    // otherwise it returns an empty vector (the UI will just use its last received frame).
    bool processBlock(const juce::AudioBuffer<float>& buffer, SpectrumData& outData);

    static constexpr int FFTOrder = 11;
    static constexpr int FFTSize = 1 << FFTOrder;
    static constexpr int NumBins = FFTSize / 2;

private:
    double currentSampleRate = 48000.0;
    
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    
    std::vector<float> fifo;
    std::vector<float> fftData;
    std::vector<float> smoothedMagnitudes;
    
    int fifoIndex = 0;
    
    float decayRate = 0.9f;

    void processFFT();
};
