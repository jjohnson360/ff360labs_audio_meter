#include "SpectrumDSP.h"
#include <cmath>
#include <algorithm>
#include <numeric>

SpectrumDSP::SpectrumDSP()
{
    initBuffers();
}

void SpectrumDSP::initBuffers()
{
    fft    = std::make_unique<juce::dsp::FFT>(fftOrder);
    window = std::make_unique<juce::dsp::WindowingFunction<float>>(
                 static_cast<size_t>(fftSize),
                 juce::dsp::WindowingFunction<float>::hann);

    fifoL.assign(static_cast<size_t>(fftSize), 0.0f);
    fifoR.assign(static_cast<size_t>(fftSize), 0.0f);
    fftDataL.assign(static_cast<size_t>(fftSize) * 2, 0.0f);
    fftDataR.assign(static_cast<size_t>(fftSize) * 2, 0.0f);
    smoothedMagnitudesL.assign(static_cast<size_t>(numBins), -100.0f);
    smoothedMagnitudesR.assign(static_cast<size_t>(numBins), -100.0f);
    fifoIndex = 0;
}

void SpectrumDSP::prepare(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    initBuffers();

    // 0.18s time constant for smoother peak decay (Phase 10.5)
    float framesPerSecond = static_cast<float>(sampleRate) / static_cast<float>(fftSize);
    if (framesPerSecond > 0)
        decayRate = std::exp(-1.0f / (0.18f * framesPerSecond));
}

void SpectrumDSP::setFFTResolution(FFTResolution resolution)
{
    currentResolution = resolution;
    fftOrder = static_cast<int>(resolution);
    fftSize  = 1 << fftOrder;
    numBins  = fftSize / 2;

    float framesPerSecond = static_cast<float>(currentSampleRate) / static_cast<float>(fftSize);
    if (framesPerSecond > 0)
        decayRate = std::exp(-1.0f / (0.18f * framesPerSecond));

    initBuffers();
}

bool SpectrumDSP::processBlock(const juce::AudioBuffer<float>& buffer, SpectrumData& outData)
{
    int numChannels = buffer.getNumChannels();
    int numSamples  = buffer.getNumSamples();

    const float* channelDataL = buffer.getReadPointer(0);
    const float* channelDataR = numChannels > 1 ? buffer.getReadPointer(1) : channelDataL;

    bool newFftCalculated = false;

    for (int i = 0; i < numSamples; ++i)
    {
        fifoL[static_cast<size_t>(fifoIndex)] = channelDataL[i];
        fifoR[static_cast<size_t>(fifoIndex)] = channelDataR[i];
        ++fifoIndex;

        if (fifoIndex >= fftSize)
        {
            // Process L
            std::copy(fifoL.begin(), fifoL.end(), fftDataL.begin());
            std::fill(fftDataL.begin() + fftSize, fftDataL.end(), 0.0f);
            processFFT(fftDataL, smoothedMagnitudesL);

            // Process R (separate pass through same windowing + FFT machinery)
            std::copy(fifoR.begin(), fifoR.end(), fftDataR.begin());
            std::fill(fftDataR.begin() + fftSize, fftDataR.end(), 0.0f);
            processFFT(fftDataR, smoothedMagnitudesR);

            fifoIndex = 0;
            newFftCalculated = true;
        }
    }

    if (newFftCalculated)
    {
        outData.magnitudesL = smoothedMagnitudesL;
        outData.magnitudesR = smoothedMagnitudesR;
    }

    return newFftCalculated;
}

void SpectrumDSP::processFFT(std::vector<float>& fftData, std::vector<float>& smoothed)
{
    // Apply Hann window
    window->multiplyWithWindowingTable(fftData.data(), static_cast<size_t>(fftSize));

    // Perform FFT in-place
    fft->performFrequencyOnlyForwardTransform(fftData.data());

    // Convert to dB with peak-hold + decay smoothing
    for (int i = 0; i < numBins; ++i)
    {
        float magnitude = fftData[static_cast<size_t>(i)];
        float db = -100.0f;
        if (magnitude > 0.00001f)
            db = 20.0f * std::log10(magnitude / (static_cast<float>(fftSize) * 0.5f));

        db = std::max(-100.0f, db);

        if (db > smoothed[static_cast<size_t>(i)])
            smoothed[static_cast<size_t>(i)] = db; // instant attack
        else
            smoothed[static_cast<size_t>(i)] = smoothed[static_cast<size_t>(i)] * decayRate
                                              + db * (1.0f - decayRate);
    }

    // 3-bin moving average for visual smoothness (Phase 10.5)
    applyMovingAverage(smoothed, 3);
}

void SpectrumDSP::applyMovingAverage(std::vector<float>& data, int windowSize)
{
    if (windowSize < 2 || data.empty()) return;
    int half = windowSize / 2;
    std::vector<float> tmp(data.size());
    for (int i = 0; i < static_cast<int>(data.size()); ++i)
    {
        float sum = 0.0f;
        int count = 0;
        for (int k = -half; k <= half; ++k)
        {
            int idx = i + k;
            if (idx >= 0 && idx < static_cast<int>(data.size()))
            {
                sum += data[static_cast<size_t>(idx)];
                ++count;
            }
        }
        tmp[static_cast<size_t>(i)] = (count > 0) ? (sum / count) : data[static_cast<size_t>(i)];
    }
    data = std::move(tmp);
}
