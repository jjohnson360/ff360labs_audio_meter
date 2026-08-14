#include "SpectrumDSP.h"
#include <cmath>
#include <algorithm>

SpectrumDSP::SpectrumDSP()
    : fft(FFTOrder),
      window(FFTSize, juce::dsp::WindowingFunction<float>::hann)
{
    fifo.resize(FFTSize, 0.0f);
    fftData.resize(FFTSize * 2, 0.0f);
    smoothedMagnitudes.resize(NumBins, -100.0f);
}

void SpectrumDSP::prepare(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    fifoIndex = 0;
    std::fill(fifo.begin(), fifo.end(), 0.0f);
    std::fill(smoothedMagnitudes.begin(), smoothedMagnitudes.end(), -100.0f);
    
    // Calculate decay rate based on sample rate to have a somewhat consistent falloff
    // Decay per FFT frame (~2048 samples). 
    // If we want a half-life of 0.1 seconds, that's roughly 2-3 frames at 48kHz.
    float framesPerSecond = static_cast<float>(sampleRate) / FFTSize;
    if (framesPerSecond > 0)
        decayRate = std::exp(-1.0f / (0.1f * framesPerSecond)); // 0.1s time constant
}

bool SpectrumDSP::processBlock(const juce::AudioBuffer<float>& buffer, SpectrumData& outData)
{
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();
    
    const float* channelDataL = buffer.getReadPointer(0);
    const float* channelDataR = numChannels > 1 ? buffer.getReadPointer(1) : channelDataL;
    
    bool newFftCalculated = false;

    for (int i = 0; i < numSamples; ++i)
    {
        // Mono mix
        float mono = (channelDataL[i] + channelDataR[i]) * 0.5f;
        
        fifo[fifoIndex++] = mono;

        if (fifoIndex >= FFTSize)
        {
            processFFT();
            fifoIndex = 0;
            newFftCalculated = true;
        }
    }
    
    if (newFftCalculated)
    {
        outData.magnitudes = smoothedMagnitudes;
    }
    
    return newFftCalculated;
}

void SpectrumDSP::processFFT()
{
    // Copy FIFO to fftData
    std::copy(fifo.begin(), fifo.end(), fftData.begin());
    std::fill(fftData.begin() + FFTSize, fftData.end(), 0.0f);
    
    // Apply window
    window.multiplyWithWindowingTable(fftData.data(), FFTSize);
    
    // Perform FFT
    fft.performFrequencyOnlyForwardTransform(fftData.data());
    
    // Convert to dB and apply smoothing
    for (int i = 0; i < NumBins; ++i)
    {
        float magnitude = fftData[i];
        float db = -100.0f;
        if (magnitude > 0.00001f)
        {
            // Normalize magnitude. FFT scales by N, so we divide by N (FFTSize).
            // But windowing reduces energy, Hann window coherent gain is 0.5.
            // Power gain is 0.375. A standard scaling factor for visualizers is to divide by (FFTSize/2).
            db = 20.0f * std::log10(magnitude / (FFTSize * 0.5f));
        }
        
        db = std::max(-100.0f, db);
        
        // Peak hold + decay
        if (db > smoothedMagnitudes[i])
        {
            smoothedMagnitudes[i] = db; // Instant attack
        }
        else
        {
            // Simple decay (linear in dB space looks natural)
            smoothedMagnitudes[i] = smoothedMagnitudes[i] * decayRate + db * (1.0f - decayRate);
        }
    }
}
