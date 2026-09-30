#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

struct PhaseScopePoint { float x, y; };

// Fixed size so a frame can be built and queued on the audio thread without allocating
struct PhaseScopeData
{
    static constexpr int MaxPoints = 256;

    float correlation = 0.0f;
    int numPoints = 0;
    std::array<PhaseScopePoint, MaxPoints> samplePairs {};
};

class PhaseScopeDSP
{
public:
    PhaseScopeDSP();

    void prepare(double sampleRate, int samplesPerBlock);
    
    // Fills outData with the block's correlation and up to MaxPoints decimated L/R pairs
    void processBlock(const juce::AudioBuffer<float>& buffer, PhaseScopeData& outData);

private:
    double currentSampleRate = 48000.0;
    
    // Smoothing coefficients for correlation
    float alpha = 0.0f;
    float meanXY = 0.0f;
    float meanXX = 0.0f;
    float meanYY = 0.0f;
};
