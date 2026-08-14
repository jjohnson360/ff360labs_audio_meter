#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

struct PhaseScopePoint { float x, y; };

struct PhaseScopeData
{
    float correlation = 0.0f;
    std::vector<PhaseScopePoint> samplePairs;
};

class PhaseScopeDSP
{
public:
    PhaseScopeDSP();

    void prepare(double sampleRate, int samplesPerBlock);
    
    // Process block and return the current Phase Scope values
    PhaseScopeData processBlock(const juce::AudioBuffer<float>& buffer);

private:
    double currentSampleRate = 48000.0;
    
    // Smoothing coefficients for correlation
    float alpha = 0.0f;
    float meanXY = 0.0f;
    float meanXX = 0.0f;
    float meanYY = 0.0f;
    
    // Decimation
    static constexpr int MaxPointsPerBlock = 256;
};
