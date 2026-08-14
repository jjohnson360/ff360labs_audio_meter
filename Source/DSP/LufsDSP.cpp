#include "LufsDSP.h"
#include <cmath>
#include <algorithm>
#include <numeric>

LufsDSP::LufsDSP()
{
    integratedHistogram.resize(NumBins, 0);
    shortTermHistogram.resize(NumBins, 0);
    shortTermHistory.resize(30, 0.0f); // 3 seconds / 100ms
}

void LufsDSP::prepare(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    
    momentarySize = static_cast<int>(sampleRate * 0.4); // 400ms
    samplesPer100ms = static_cast<int>(sampleRate * 0.1); // 100ms
    
    momentaryBufferL.assign(momentarySize, 0.0f);
    momentaryBufferR.assign(momentarySize, 0.0f);
    
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(samplesPer100ms), 1 };
    preFilterL.prepare(spec);
    preFilterR.prepare(spec);
    highPassL.prepare(spec);
    highPassR.prepare(spec);
    
    updateFilterCoefficients();
    reset();
}

void LufsDSP::updateFilterCoefficients()
{
    // Stage 1: High shelf filter
    // f0 = 1500 Hz, Q = 0.7071, gain = 4.0 dB
    float q = 0.7071f; // 1/sqrt(2)
    auto shelfCoefs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(currentSampleRate, 1500.0f, q, std::pow(10.0f, 4.0f / 20.0f));
    
    // Stage 2: High pass filter
    // f0 = 38 Hz, Q = 0.5
    auto hpCoefs = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, 38.0f, 0.5f);
    
    preFilterL.coefficients = shelfCoefs;
    preFilterR.coefficients = shelfCoefs;
    highPassL.coefficients = hpCoefs;
    highPassR.coefficients = hpCoefs;
}

void LufsDSP::reset()
{
    std::fill(momentaryBufferL.begin(), momentaryBufferL.end(), 0.0f);
    std::fill(momentaryBufferR.begin(), momentaryBufferR.end(), 0.0f);
    momentaryIndex = 0;
    momentarySumL = 0.0f;
    momentarySumR = 0.0f;
    
    blockCounter100ms = 0;
    
    std::fill(integratedHistogram.begin(), integratedHistogram.end(), 0);
    std::fill(shortTermHistogram.begin(), shortTermHistogram.end(), 0);
    std::fill(shortTermHistory.begin(), shortTermHistory.end(), 0.0f);
    shortTermHistoryIndex = 0;
    
    currentMomentary = -70.0f;
    currentShortTerm = -70.0f;
    currentIntegrated = -70.0f;
    currentLra = 0.0f;
    
    preFilterL.reset();
    preFilterR.reset();
    highPassL.reset();
    highPassR.reset();
}

float LufsDSP::calcLufs(float powerSum)
{
    if (powerSum <= 0.0f) return -70.0f;
    float lufs = -0.691f + 10.0f * std::log10(powerSum);
    return std::max(-70.0f, lufs);
}

void LufsDSP::processSample(float sampleL, float sampleR)
{
    // Process K-weighting
    float filteredL = highPassL.processSample(preFilterL.processSample(sampleL));
    float filteredR = highPassR.processSample(preFilterR.processSample(sampleR));
    
    // Square
    float sqL = filteredL * filteredL;
    float sqR = filteredR * filteredR;
    
    // Update momentary sliding window sum
    momentarySumL -= momentaryBufferL[momentaryIndex];
    momentarySumR -= momentaryBufferR[momentaryIndex];
    
    momentaryBufferL[momentaryIndex] = sqL;
    momentaryBufferR[momentaryIndex] = sqR;
    
    momentarySumL += sqL;
    momentarySumR += sqR;
    
    momentaryIndex = (momentaryIndex + 1) % momentarySize;
    
    // Tick 100ms blocks for Integrated / Short-Term
    blockCounter100ms++;
    if (blockCounter100ms >= samplesPer100ms)
    {
        process100msBlock();
        blockCounter100ms = 0;
    }
}

void LufsDSP::process100msBlock()
{
    // 1. Calculate momentary power for the last 400ms
    float powerL = momentarySumL / momentarySize;
    float powerR = momentarySumR / momentarySize;
    float momentaryPower = powerL + powerR; // unweighted sum for stereo (both w_i = 1.0)
    
    currentMomentary = calcLufs(momentaryPower);
    
    // 2. Add to short term history (store power, not LUFS, so we can average it over 3s)
    shortTermHistory[shortTermHistoryIndex] = momentaryPower;
    shortTermHistoryIndex = (shortTermHistoryIndex + 1) % 30; // 30 * 100ms = 3s
    
    // Calculate short term power
    float stPowerSum = 0.0f;
    for (float p : shortTermHistory) stPowerSum += p;
    float shortTermPower = stPowerSum / 30.0f;
    
    currentShortTerm = calcLufs(shortTermPower);
    
    // 3. Update Histograms
    auto getBin = [](float lufs) -> int {
        int bin = static_cast<int>((lufs + 70.0f) * 10.0f);
        return juce::jlimit(0, NumBins - 1, bin);
    };
    
    if (currentMomentary >= AbsoluteGate)
    {
        integratedHistogram[getBin(currentMomentary)]++;
        currentIntegrated = calculateIntegratedFromHistogram();
    }
    
    if (currentShortTerm >= AbsoluteGate)
    {
        shortTermHistogram[getBin(currentShortTerm)]++;
        currentLra = calculateLraFromHistogram();
    }
}

float LufsDSP::calculateIntegratedFromHistogram()
{
    // 1. Calculate ungated absolute power sum
    double absSum = 0.0;
    int absCount = 0;
    for (int i = 0; i < NumBins; ++i)
    {
        if (integratedHistogram[i] > 0)
        {
            float lufs = (i / 10.0f) - 70.0f;
            double power = std::pow(10.0, (lufs + 0.691f) / 10.0f);
            absSum += power * integratedHistogram[i];
            absCount += integratedHistogram[i];
        }
    }
    
    if (absCount == 0) return -70.0f;
    
    float absLufs = calcLufs(static_cast<float>(absSum / absCount));
    float relativeGate = absLufs - 10.0f;
    
    // 2. Calculate relative gated power
    double relSum = 0.0;
    int relCount = 0;
    int relativeGateBin = static_cast<int>((relativeGate + 70.0f) * 10.0f);
    relativeGateBin = std::max(0, relativeGateBin);
    
    for (int i = relativeGateBin; i < NumBins; ++i)
    {
        if (integratedHistogram[i] > 0)
        {
            float lufs = (i / 10.0f) - 70.0f;
            double power = std::pow(10.0, (lufs + 0.691f) / 10.0f);
            relSum += power * integratedHistogram[i];
            relCount += integratedHistogram[i];
        }
    }
    
    if (relCount == 0) return -70.0f;
    return calcLufs(static_cast<float>(relSum / relCount));
}

float LufsDSP::calculateLraFromHistogram()
{
    // Find the relative gate for LRA (Absolute LUFS - 20 LU)
    double absSum = 0.0;
    int absCount = 0;
    for (int i = 0; i < NumBins; ++i)
    {
        if (shortTermHistogram[i] > 0)
        {
            float lufs = (i / 10.0f) - 70.0f;
            double power = std::pow(10.0, (lufs + 0.691f) / 10.0f);
            absSum += power * shortTermHistogram[i];
            absCount += shortTermHistogram[i];
        }
    }
    
    if (absCount == 0) return 0.0f;
    
    float absLufs = calcLufs(static_cast<float>(absSum / absCount));
    float relativeGate = absLufs - 20.0f;
    int relativeGateBin = static_cast<int>((relativeGate + 70.0f) * 10.0f);
    relativeGateBin = std::max(0, relativeGateBin);
    
    int relCount = 0;
    for (int i = relativeGateBin; i < NumBins; ++i)
        relCount += shortTermHistogram[i];
        
    if (relCount == 0) return 0.0f;
    
    // Discard top 5% and bottom 10%
    int discardBottom = static_cast<int>(relCount * 0.10f);
    int discardTop = static_cast<int>(relCount * 0.05f);
    
    float lowLufs = -70.0f;
    float highLufs = -70.0f;
    
    int currentSum = 0;
    for (int i = relativeGateBin; i < NumBins; ++i)
    {
        if (shortTermHistogram[i] > 0)
        {
            currentSum += shortTermHistogram[i];
            if (currentSum > discardBottom && lowLufs == -70.0f)
                lowLufs = (i / 10.0f) - 70.0f;
                
            if (currentSum >= (relCount - discardTop))
            {
                highLufs = (i / 10.0f) - 70.0f;
                break;
            }
        }
    }
    
    return std::max(0.0f, highLufs - lowLufs);
}

LufsMeterData LufsDSP::processBlock(const juce::AudioBuffer<float>& buffer)
{
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();
    
    const float* channelDataL = buffer.getReadPointer(0);
    const float* channelDataR = numChannels > 1 ? buffer.getReadPointer(1) : channelDataL;
    
    for (int i = 0; i < numSamples; ++i)
    {
        processSample(channelDataL[i], channelDataR[i]);
    }
    
    LufsMeterData data;
    data.momentary = currentMomentary;
    data.shortTerm = currentShortTerm;
    data.integrated = currentIntegrated;
    data.lra = currentLra;
    
    return data;
}
