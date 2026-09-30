#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
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

    // Audio thread (or before playback) only
    void reset();

    // Thread-safe: the audio thread performs the reset at the start of its next block
    void requestReset() { resetRequested.store(true); }

    // Thread-safe readers for the UI / report export
    float getMomentary() const  { return currentMomentary.load(); }
    float getShortTerm() const  { return currentShortTerm.load(); }
    float getIntegrated() const { return currentIntegrated.load(); }
    float getLRA() const        { return currentLra.load(); }
    float getMaxMomentary() const { return maxMomentary.load(); }
    float getMaxShortTerm() const { return maxShortTerm.load(); }

private:
    double currentSampleRate = 48000.0;

    // K-Weighting Filters per channel (Left, Right)
    juce::dsp::IIR::Filter<float> preFilterL;
    juce::dsp::IIR::Filter<float> highPassL;
    juce::dsp::IIR::Filter<float> preFilterR;
    juce::dsp::IIR::Filter<float> highPassR;

    // Buffers for Momentary (400ms). Running sums are double: a float sum
    // updated per sample drifts over a long session.
    std::vector<float> momentaryBufferL;
    std::vector<float> momentaryBufferR;
    int momentarySize = 0;
    int momentaryIndex = 0;
    double momentarySumL = 0.0;
    double momentarySumR = 0.0;
    bool isMono = false;

    // For overlapping 400ms blocks (calculated every 100ms) to compute Integrated/LRA
    int blockCounter100ms = 0;
    int samplesPer100ms = 0;

    // Histogram for Integrated/LRA (Bins of 0.1 LU, from -70 to +10)
    static constexpr int NumBins = 800;
    std::vector<int> integratedHistogram;
    std::vector<int> shortTermHistogram;
    std::array<double, NumBins> binPower {}; // mean-square power at each bin centre

    // Gating parameters
    static constexpr float AbsoluteGate = -70.0f;

    // Recent outputs
    std::atomic<float> currentMomentary { -70.0f };
    std::atomic<float> currentShortTerm { -70.0f };
    std::atomic<float> currentIntegrated { -70.0f };
    std::atomic<float> currentLra { 0.0f };
    std::atomic<float> maxMomentary { -70.0f };
    std::atomic<float> maxShortTerm { -70.0f };
    std::atomic<bool> resetRequested { false };

    // Short-term accumulation (3s = 30 blocks of 100ms)
    std::vector<float> shortTermHistory;
    int shortTermHistoryIndex = 0;

    // 100 ms steps since the last reset: gating (BS.1770) and LRA (EBU Tech 3342) only count
    // complete 400 ms / 3 s windows, not the partial ones right after a start or reset
    int blocksSinceReset = 0;

    void updateFilterCoefficients();
    void processSample(float sampleL, float sampleR);
    void process100msBlock();

    float calculateIntegratedFromHistogram();
    float calculateLraFromHistogram();

    static float calcLufs(double powerSum);
    static int getBin(float lufs);
};
