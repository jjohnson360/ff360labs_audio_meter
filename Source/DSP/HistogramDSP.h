#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <array>

struct HistogramData
{
    std::array<int, 71> bins; // -70 to 0 LUFS
    int maxCount = 0;
    int modalBinIndex = 0;
};

class HistogramDSP
{
public:
    HistogramDSP();

    void prepare(double sampleRate, int samplesPerBlock);
    
    // Process block using the lufs value provided.
    // Returns true if the histogram updated (every 100ms), in which case outData is filled.
    bool processBlock(const juce::AudioBuffer<float>& buffer, float currentLufs, HistogramData& outData);

    void reset();

private:
    double currentSampleRate = 48000.0;
    int samplesPer100ms = 4800;
    int sampleCounter = 0;
    
    // 5 minutes at 10Hz (100ms updates) = 3000 values
    static constexpr int HistorySize = 3000;
    std::vector<float> history;
    int historyIndex = 0;
    int historyCount = 0;

    void updateHistogram(HistogramData& outData);
};
