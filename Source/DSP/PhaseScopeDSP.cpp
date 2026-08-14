#include "PhaseScopeDSP.h"
#include <cmath>
#include <algorithm>

PhaseScopeDSP::PhaseScopeDSP()
{
}

void PhaseScopeDSP::prepare(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    
    // 300ms integration time for correlation meter
    float tau = 0.3f;
    if (sampleRate > 0)
        alpha = 1.0f - std::exp(-1.0f / (tau * static_cast<float>(sampleRate)));
        
    meanXY = 0.0f;
    meanXX = 0.0f;
    meanYY = 0.0f;
}

PhaseScopeData PhaseScopeDSP::processBlock(const juce::AudioBuffer<float>& buffer)
{
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();
    
    const float* channelDataL = buffer.getReadPointer(0);
    const float* channelDataR = numChannels > 1 ? buffer.getReadPointer(1) : channelDataL;
    
    PhaseScopeData data;
    
    // Decimation step size to limit points drawn per frame
    int step = std::max(1, numSamples / MaxPointsPerBlock);
    data.samplePairs.reserve(numSamples / step + 1);
    
    for (int i = 0; i < numSamples; ++i)
    {
        float l = channelDataL[i];
        float r = channelDataR[i];
        
        // IIR smoothing for correlation calculation
        meanXY += alpha * (l * r - meanXY);
        meanXX += alpha * (l * l - meanXX);
        meanYY += alpha * (r * r - meanYY);
        
        if (i % step == 0)
        {
            data.samplePairs.push_back({l, r});
        }
    }
    
    // Calculate final correlation coefficient [-1.0, 1.0]
    float denominator = std::sqrt(meanXX * meanYY);
    if (denominator > 0.00001f)
    {
        data.correlation = juce::jlimit(-1.0f, 1.0f, meanXY / denominator);
    }
    else
    {
        data.correlation = 0.0f; // Silence
    }
    
    return data;
}
