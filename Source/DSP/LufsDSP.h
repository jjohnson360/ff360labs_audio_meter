#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>

struct LufsMeterData
{
    float momentary = -70.0f;
    float shortTerm = -70.0f;
    float integrated = -70.0f;
    float lra = 0.0f;
};

class LufsDSP
{
public:
    LufsDSP();

    void prepare(double sampleRate, int samplesPerBlock);
    
    // Process block and return the current LUFS values
    LufsMeterData processBlock(const juce::AudioBuffer<float>& buffer);

    void reset();

    float getMomentary() const { return currentMomentary; }
    float getShortTerm() const { return currentShortTerm; }
    float getIntegrated() const { return currentIntegrated; }
    float getLRA() const { return currentLra; }

private:
    double currentSampleRate = 48000.0;
    
    // K-Weighting Filters per channel (Left, Right)
    juce::dsp::IIR::Filter<float> preFilterL;
    juce::dsp::IIR::Filter<float> highPassL;
    juce::dsp::IIR::Filter<float> preFilterR;
    juce::dsp::IIR::Filter<float> highPassR;

    // Buffers for Momentary (400ms) and Short-term (3s)
    std::vector<float> momentaryBufferL;
    std::vector<float> momentaryBufferR;
    int momentarySize = 0;
    int momentaryIndex = 0;
    float momentarySumL = 0.0f;
    float momentarySumR = 0.0f;
    
    // For overlapping 400ms blocks (calculated every 100ms) to compute Integrated/LRA
    int blockCounter100ms = 0;
    int samplesPer100ms = 0;
    
    // Histogram for Integrated/LRA (Bins of 0.1 LU, from -70 to +10)
    static constexpr int NumBins = 800; 
    std::vector<int> integratedHistogram;
    std::vector<int> shortTermHistogram;
    
    // Gating parameters
    static constexpr float AbsoluteGate = -70.0f;
    
    // Recent outputs
    float currentMomentary = -70.0f;
    float currentShortTerm = -70.0f;
    float currentIntegrated = -70.0f;
    float currentLra = 0.0f;

    // Short-term accumulation (3s = 30 blocks of 100ms)
    std::vector<float> shortTermHistory;
    int shortTermHistoryIndex = 0;

    void updateFilterCoefficients();
    void processSample(float sampleL, float sampleR);
    void process100msBlock();
    
    float calculateIntegratedFromHistogram();
    float calculateLraFromHistogram();
    
    float calcLufs(float powerSum);
};
