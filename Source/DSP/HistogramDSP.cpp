#include "HistogramDSP.h"
#include <cmath>
#include <algorithm>

HistogramDSP::HistogramDSP()
{
    history.resize(HistorySize, -70.0f);
}

void HistogramDSP::prepare(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    samplesPer100ms = static_cast<int>(sampleRate * 0.1);
    reset();
}

void HistogramDSP::reset()
{
    sampleCounter = 0;
    historyIndex = 0;
    historyCount = 0;
    std::fill(history.begin(), history.end(), -70.0f);
}

bool HistogramDSP::processBlock(const juce::AudioBuffer<float>& buffer, float currentLufs, HistogramData& outData)
{
    int numSamples = buffer.getNumSamples();
    bool updated = false;
    
    for (int i = 0; i < numSamples; ++i)
    {
        sampleCounter++;
        if (sampleCounter >= samplesPer100ms)
        {
            sampleCounter = 0;
            
            // Record value
            history[historyIndex] = currentLufs;
            historyIndex = (historyIndex + 1) % HistorySize;
            if (historyCount < HistorySize) historyCount++;
            
            updated = true;
        }
    }
    
    if (updated)
    {
        updateHistogram(outData);
    }
    
    return updated;
}

void HistogramDSP::updateHistogram(HistogramData& outData)
{
    outData.bins.fill(0);
    outData.maxCount = 0;
    outData.modalBinIndex = 0;
    
    if (historyCount == 0) return;
    
    for (int i = 0; i < historyCount; ++i)
    {
        float val = history[i];
        
        // Bin calculation: map -70 to 0, -69 to 1, ..., 0 to 70.
        int bin = static_cast<int>(std::round(val + 70.0f));
        bin = juce::jlimit(0, 70, bin);
        
        outData.bins[bin]++;
        
        if (outData.bins[bin] > outData.maxCount)
        {
            outData.maxCount = outData.bins[bin];
            outData.modalBinIndex = bin;
        }
    }
}
