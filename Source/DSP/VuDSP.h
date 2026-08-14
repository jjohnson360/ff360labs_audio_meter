#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

struct VuMeterData
{
    float vuL = -60.0f;
    float vuR = -60.0f;
};

class VuDSP
{
public:
    static constexpr float DEFAULT_VU_ATTACK_TIME_SEC  = 0.12f; // ~120ms fast rise time
    static constexpr float DEFAULT_VU_RELEASE_TIME_SEC = 0.35f; // ~350ms smooth decay time

    VuDSP();

    void prepare(double sampleRate, int samplesPerBlock);
    
    // Process block and return the current VU values (in pseudo-dB relative to 0 VU)
    VuMeterData processBlock(const juce::AudioBuffer<float>& buffer);

    // Set the reference level where 0 VU corresponds to (default -18 dBFS)
    void setReferenceLevelDb(float refLevelDb);

    // Expose adjustable ballistics
    void setBallistics(float attackSec, float releaseSec);

    static float gainToVuDb(float linearGain);

private:
    double currentSampleRate = 48000.0;
    float referenceLevelDb = -18.0f;
    float referenceGain = 0.12589f; // 10^(-18/20)
    
    float attackTimeSec = DEFAULT_VU_ATTACK_TIME_SEC;
    float releaseTimeSec = DEFAULT_VU_RELEASE_TIME_SEC;
    float alphaAttack = 0.0f;
    float alphaRelease = 0.0f;

    float stateL = 0.0f;
    float stateR = 0.0f;

    void updateCoefficients();
};
