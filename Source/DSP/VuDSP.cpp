#include "VuDSP.h"
#include <cmath>
#include <algorithm>

VuDSP::VuDSP()
{
    setReferenceLevelDb(-18.0f);
}

void VuDSP::prepare(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    statePowerL = 0.0f;
    statePowerR = 0.0f;
    updateCoefficients();
}

void VuDSP::setReferenceLevelDb(float refLevelDb)
{
    referenceLevelDb = refLevelDb;
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
    
    if (numChannels == 0 || numSamples == 0)
    {
        VuMeterData empty;
        return empty;
    }

    const float* channelDataL = buffer.getReadPointer(0);
    const float* channelDataR = numChannels > 1 ? buffer.getReadPointer(1) : channelDataL;

    for (int i = 0; i < numSamples; ++i)
    {
        // Compute instantaneous power (x^2)
        float inL = channelDataL[i];
        float inR = channelDataR[i];
        float powerL = inL * inL;
        float powerR = inR * inR;

        // Continuous IIR integration on power with asymmetric ballistics
        float aL = (powerL > statePowerL) ? alphaAttack : alphaRelease;
        statePowerL += aL * (powerL - statePowerL);

        float aR = (powerR > statePowerR) ? alphaAttack : alphaRelease;
        statePowerR += aR * (powerR - statePowerR);
    }

    // AES-17 standard: full-scale sine peak 1.0 has power 0.5, AES-17 RMS = sqrt(2 * mean_square) = 1.0 (0 dBFS)
    float rmsL = std::sqrt(std::max(0.0f, statePowerL * 2.0f));
    float rmsR = std::sqrt(std::max(0.0f, statePowerR * 2.0f));

    // Convert RMS to dBFS
    float dbfsL = (rmsL > 1e-5f) ? (20.0f * std::log10(rmsL)) : -60.0f;
    float dbfsR = (rmsR > 1e-5f) ? (20.0f * std::log10(rmsR)) : -60.0f;

    VuMeterData data;
    // 0 VU is at referenceLevelDb (e.g. -18 dBFS)
    // When dbfsL == -18.0 and ref == -18.0, vuL == 0.0 VU
    data.vuL = std::clamp(dbfsL - referenceLevelDb, -60.0f, 10.0f);
    data.vuR = std::clamp(dbfsR - referenceLevelDb, -60.0f, 10.0f);
    data.rawDbfslL = dbfsL;
    data.rawDbfslR = dbfsR;

    return data;
}

float VuDSP::gainToVuDb(float linearGain)
{
    if (linearGain <= 0.00001f) // Avoid -inf
        return -60.0f;
        
    return 20.0f * std::log10(linearGain);
}

