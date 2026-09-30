#include "LufsDSP.h"
#include <cmath>
#include <algorithm>
#include <numeric>

LufsDSP::LufsDSP()
{
    integratedHistogram.resize(NumBins, 0);
    shortTermHistogram.resize(NumBins, 0);
    shortTermHistory.resize(30, 0.0f); // 3 seconds / 100ms

    // Power at each 0.1 LU bin's centre, so the gating sums don't call pow() per bin every 100 ms
    for (int i = 0; i < NumBins; ++i)
    {
        const double lufs = (i + 0.5) / 10.0 - 70.0;
        binPower[(size_t) i] = std::pow(10.0, (lufs + 0.691) / 10.0);
    }
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
    resetRequested.store(false);

    std::fill(momentaryBufferL.begin(), momentaryBufferL.end(), 0.0f);
    std::fill(momentaryBufferR.begin(), momentaryBufferR.end(), 0.0f);
    momentaryIndex = 0;
    momentarySumL = 0.0;
    momentarySumR = 0.0;

    blockCounter100ms = 0;

    std::fill(integratedHistogram.begin(), integratedHistogram.end(), 0);
    std::fill(shortTermHistogram.begin(), shortTermHistogram.end(), 0);
    std::fill(shortTermHistory.begin(), shortTermHistory.end(), 0.0f);
    shortTermHistoryIndex = 0;
    blocksSinceReset = 0;

    currentMomentary.store(-70.0f);
    currentShortTerm.store(-70.0f);
    currentIntegrated.store(-70.0f);
    currentLra.store(0.0f);
    maxMomentary.store(-70.0f);
    maxShortTerm.store(-70.0f);

    preFilterL.reset();
    preFilterR.reset();
    highPassL.reset();
    highPassR.reset();
}

float LufsDSP::calcLufs(double powerSum)
{
    if (powerSum <= 0.0) return -70.0f;
    float lufs = static_cast<float>(-0.691 + 10.0 * std::log10(powerSum));
    return std::max(-70.0f, lufs);
}

int LufsDSP::getBin(float lufs)
{
    int bin = static_cast<int>(std::floor((lufs + 70.0f) * 10.0f));
    return juce::jlimit(0, NumBins - 1, bin);
}

void LufsDSP::processSample(float sampleL, float sampleR)
{
    // Process K-weighting
    float filteredL = highPassL.processSample(preFilterL.processSample(sampleL));
    float filteredR = isMono ? 0.0f : highPassR.processSample(preFilterR.processSample(sampleR));

    // Square
    float sqL = filteredL * filteredL;
    float sqR = filteredR * filteredR;

    // Update momentary sliding window sum
    momentarySumL -= momentaryBufferL[(size_t) momentaryIndex];
    momentarySumR -= momentaryBufferR[(size_t) momentaryIndex];

    momentaryBufferL[(size_t) momentaryIndex] = sqL;
    momentaryBufferR[(size_t) momentaryIndex] = sqR;

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
    ++blocksSinceReset;
    const bool momentaryWindowFull = blocksSinceReset >= 4;   // 400 ms
    const bool shortTermWindowFull = blocksSinceReset >= 30;  // 3 s

    // 1. Calculate momentary power for the last 400ms
    // BS.1770 sums the channels' mean squares (L and R both weighted 1.0). A mono input
    // is one channel: counting it as both L and R, as before, read 3 dB hot.
    double powerL = std::max(0.0, momentarySumL) / momentarySize;
    double powerR = std::max(0.0, momentarySumR) / momentarySize;
    double momentaryPower = powerL + powerR;

    const float momentary = calcLufs(momentaryPower);
    currentMomentary.store(momentary);
    if (momentary > maxMomentary.load())
        maxMomentary.store(momentary);

    // 2. Add to short term history (store power, not LUFS, so we can average it over 3s)
    shortTermHistory[(size_t) shortTermHistoryIndex] = static_cast<float>(momentaryPower);
    shortTermHistoryIndex = (shortTermHistoryIndex + 1) % 30; // 30 * 100ms = 3s

    // Calculate short term power
    double stPowerSum = 0.0;
    for (float p : shortTermHistory) stPowerSum += p;

    const float shortTerm = calcLufs(stPowerSum / 30.0);
    currentShortTerm.store(shortTerm);
    if (shortTerm > maxShortTerm.load())
        maxShortTerm.store(shortTerm);

    // 3. Update Histograms
    if (momentaryWindowFull && momentary > AbsoluteGate)
    {
        integratedHistogram[(size_t) getBin(momentary)]++;
        currentIntegrated.store(calculateIntegratedFromHistogram());
    }

    if (shortTermWindowFull && shortTerm > AbsoluteGate)
    {
        shortTermHistogram[(size_t) getBin(shortTerm)]++;
        currentLra.store(calculateLraFromHistogram());
    }
}

float LufsDSP::calculateIntegratedFromHistogram()
{
    // 1. Calculate ungated absolute power sum
    double absSum = 0.0;
    int absCount = 0;
    for (int i = 0; i < NumBins; ++i)
    {
        if (integratedHistogram[(size_t) i] > 0)
        {
            absSum += binPower[(size_t) i] * integratedHistogram[(size_t) i];
            absCount += integratedHistogram[(size_t) i];
        }
    }

    if (absCount == 0) return -70.0f;

    float absLufs = calcLufs(absSum / absCount);
    float relativeGate = absLufs - 10.0f;

    // 2. Calculate relative gated power
    double relSum = 0.0;
    int relCount = 0;
    int relativeGateBin = static_cast<int>((relativeGate + 70.0f) * 10.0f);
    relativeGateBin = std::max(0, relativeGateBin);

    for (int i = relativeGateBin; i < NumBins; ++i)
    {
        if (integratedHistogram[(size_t) i] > 0)
        {
            relSum += binPower[(size_t) i] * integratedHistogram[(size_t) i];
            relCount += integratedHistogram[(size_t) i];
        }
    }

    if (relCount == 0) return -70.0f;
    return calcLufs(relSum / relCount);
}

float LufsDSP::calculateLraFromHistogram()
{
    // Find the relative gate for LRA (Absolute LUFS - 20 LU)
    double absSum = 0.0;
    int absCount = 0;
    for (int i = 0; i < NumBins; ++i)
    {
        if (shortTermHistogram[(size_t) i] > 0)
        {
            absSum += binPower[(size_t) i] * shortTermHistogram[(size_t) i];
            absCount += shortTermHistogram[(size_t) i];
        }
    }

    if (absCount == 0) return 0.0f;

    float absLufs = calcLufs(absSum / absCount);
    float relativeGate = absLufs - 20.0f;
    int relativeGateBin = static_cast<int>((relativeGate + 70.0f) * 10.0f);
    relativeGateBin = std::max(0, relativeGateBin);

    int relCount = 0;
    for (int i = relativeGateBin; i < NumBins; ++i)
        relCount += shortTermHistogram[(size_t) i];

    if (relCount == 0) return 0.0f;

    // Discard top 5% and bottom 10%
    int discardBottom = static_cast<int>(relCount * 0.10f);
    int discardTop = static_cast<int>(relCount * 0.05f);

    int lowBin = -1;
    int highBin = -1;

    int currentSum = 0;
    for (int i = relativeGateBin; i < NumBins; ++i)
    {
        if (shortTermHistogram[(size_t) i] > 0)
        {
            currentSum += shortTermHistogram[(size_t) i];
            if (currentSum > discardBottom && lowBin < 0)
                lowBin = i;

            if (currentSum >= (relCount - discardTop))
            {
                highBin = i;
                break;
            }
        }
    }

    if (lowBin < 0 || highBin < 0)
        return 0.0f;

    return std::max(0.0f, (highBin - lowBin) / 10.0f);
}

LufsMeterData LufsDSP::processBlock(const juce::AudioBuffer<float>& buffer)
{
    if (resetRequested.load())
        reset();

    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    if (numChannels > 0 && momentarySize > 0)
    {
        isMono = (numChannels == 1);

        const float* channelDataL = buffer.getReadPointer(0);
        const float* channelDataR = numChannels > 1 ? buffer.getReadPointer(1) : channelDataL;

        for (int i = 0; i < numSamples; ++i)
            processSample(channelDataL[i], channelDataR[i]);
    }

    LufsMeterData data;
    data.momentary = currentMomentary.load();
    data.shortTerm = currentShortTerm.load();
    data.integrated = currentIntegrated.load();
    data.lra = currentLra.load();

    return data;
}
