#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

struct MeterData
{
    float peakL = -60.0f;
    float peakR = -60.0f;
    float rmsL = -60.0f;
    float rmsR = -60.0f;
};

class PeakRmsDSP
{
public:
    PeakRmsDSP();

    void prepare(double sampleRate, int samplesPerBlock);
    
    // Calculates un-smoothed block peak/RMS and returns it
    MeterData processBlock(const juce::AudioBuffer<float>& buffer);

    // Ballistics logic, takes raw block target data and returns smoothed values based on attack/decay
    void applyBallistics(MeterData& currentSmoothedData, const MeterData& targetData);

    static float gainToDb(float linearGain);

private:
    double currentSampleRate = 48000.0;

    // Decay rate calculated from release time (e.g., 300ms)
    float attackCoef = 0.0f;
    float releaseCoef = 0.0f;

    void updateCoefficients();
};
