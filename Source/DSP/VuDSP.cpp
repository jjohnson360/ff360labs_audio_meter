#include "VuDSP.h"
#include <cmath>

VuDSP::VuDSP()
{
    setReferenceLevelDb(-18.0f);
}

void VuDSP::prepare(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    stateL = 0.0f;
    stateR = 0.0f;
    updateCoefficients();
}

void VuDSP::setReferenceLevelDb(float refLevelDb)
{
    referenceLevelDb = refLevelDb;
    referenceGain = std::pow(10.0f, referenceLevelDb / 20.0f);
}

void VuDSP::setBallistics(float attackSec, float releaseSec)
{
    attackTimeSec = attackSec;
    releaseTimeSec = releaseSec;
    updateCoefficients();
}

void VuDSP::updateCoefficients()
{
    if (currentSampleRate > 0)
    {
        alphaAttack  = 1.0f - std::exp(-1.0f / (attackTimeSec * static_cast<float>(currentSampleRate)));
        alphaRelease = 1.0f - std::exp(-1.0f / (releaseTimeSec * static_cast<float>(currentSampleRate)));
    }
}

VuMeterData VuDSP::processBlock(const juce::AudioBuffer<float>& buffer)
{
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();
    
    const float* channelDataL = buffer.getReadPointer(0);
    const float* channelDataR = numChannels > 1 ? buffer.getReadPointer(1) : channelDataL;

    for (int i = 0; i < numSamples; ++i)
    {
        // Rectify input
        float absL = std::abs(channelDataL[i]);
        float absR = std::abs(channelDataR[i]);

        // Asymmetric One-pole IIR filter (120ms attack / 350ms release)
        float aL = (absL > stateL) ? alphaAttack : alphaRelease;
        stateL += aL * (absL - stateL);

        float aR = (absR > stateR) ? alphaAttack : alphaRelease;
        stateR += aR * (absR - stateR);
    }

    VuMeterData data;
    // Scale by reference level so that 0 VU = referenceLevelDb
    // Convert to pseudo-dB relative to reference
    float scaledGainL = stateL / referenceGain;
    float scaledGainR = stateR / referenceGain;
    
    data.vuL = gainToVuDb(scaledGainL);
    data.vuR = gainToVuDb(scaledGainR);

    return data;
}

float VuDSP::gainToVuDb(float linearGain)
{
    if (linearGain <= 0.00001f) // Avoid -inf
        return -60.0f;
        
    return 20.0f * std::log10(linearGain);
}
