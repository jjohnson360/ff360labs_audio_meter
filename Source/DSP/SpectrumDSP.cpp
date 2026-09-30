#include "SpectrumDSP.h"
#include <cmath>
#include <algorithm>

SpectrumDSP::SpectrumDSP()
{
    for (int i = 0; i < NumOrders; ++i)
    {
        const int size = 1 << (MinOrder + i);
        ffts[(size_t) i] = std::make_unique<juce::dsp::FFT>(MinOrder + i);
        windows[(size_t) i].resize((size_t) size);
        juce::dsp::WindowingFunction<float>::fillWindowingTables(windows[(size_t) i].data(), (size_t) size,
                                                                 juce::dsp::WindowingFunction<float>::hann, false);
    }

    fifoL.assign((size_t) MaxFFTSize, 0.0f);
    fifoR.assign((size_t) MaxFFTSize, 0.0f);
    fftData.assign((size_t) MaxFFTSize * 2, 0.0f);
    smoothedMagnitudesL.assign((size_t) MaxFFTSize / 2, -100.0f);
    smoothedMagnitudesR.assign((size_t) MaxFFTSize / 2, -100.0f);
}

void SpectrumDSP::resetState()
{
    fifoIndex = 0;
    std::fill(smoothedMagnitudesL.begin(), smoothedMagnitudesL.end(), -100.0f);
    std::fill(smoothedMagnitudesR.begin(), smoothedMagnitudesR.end(), -100.0f);
}

void SpectrumDSP::updateDecayRate()
{
    // 0.18s time constant for smoother peak decay (Phase 10.5)
    float framesPerSecond = static_cast<float>(currentSampleRate) / static_cast<float>(getFFTSize());
    if (framesPerSecond > 0)
        decayRate = std::exp(-1.0f / (0.18f * framesPerSecond));
}

void SpectrumDSP::prepare(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    activeOrder = requestedOrder.load();
    updateDecayRate();
    resetState();
}

void SpectrumDSP::setFFTResolution(FFTResolution resolution)
{
    requestedOrder.store(juce::jlimit(MinOrder, MaxOrder, static_cast<int>(resolution)));
}

bool SpectrumDSP::processBlock(const juce::AudioBuffer<float>& buffer, SpectrumData& outData)
{
    int numChannels = buffer.getNumChannels();
    int numSamples  = buffer.getNumSamples();
    if (numChannels == 0 || numSamples == 0)
        return false;

    if (const int order = requestedOrder.load(); order != activeOrder)
    {
        activeOrder = order;
        updateDecayRate();
        resetState();
    }

    const int fftSize = getFFTSize();
    const int numBins = getNumBins();

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
            processFFT(fifoL, smoothedMagnitudesL);
            if (numChannels > 1)
                processFFT(fifoR, smoothedMagnitudesR);

            fifoIndex = 0;
            newFftCalculated = true;
        }
    }

    if (newFftCalculated)
    {
        outData.numBins = numBins;
        outData.hasRight = numChannels > 1;
        outData.sampleRate = currentSampleRate;

        // 3-bin smoothing for visual smoothness (Phase 10.5). Applied to the output
        // only: smoothing the peak-hold state itself would blur it further every frame.
        applyMovingAverage(smoothedMagnitudesL.data(), outData.magnitudesL.data(), numBins);
        if (outData.hasRight)
            applyMovingAverage(smoothedMagnitudesR.data(), outData.magnitudesR.data(), numBins);
    }

    return newFftCalculated;
}

void SpectrumDSP::processFFT(const std::vector<float>& fifo, std::vector<float>& smoothed)
{
    const int fftSize = getFFTSize();
    const int numBins = getNumBins();
    const auto& window = windows[(size_t) (activeOrder - MinOrder)];

    // Apply Hann window
    juce::FloatVectorOperations::multiply(fftData.data(), fifo.data(), window.data(), fftSize);
    std::fill(fftData.begin() + fftSize, fftData.begin() + fftSize * 2, 0.0f);

    // Perform FFT in-place
    ffts[(size_t) (activeOrder - MinOrder)]->performFrequencyOnlyForwardTransform(fftData.data());

    // A sine of peak amplitude A gives a bin magnitude of A * sum(window) / 2, and a Hann
    // window sums to N / 2, so dividing by N / 4 makes a 0 dBFS sine read 0 dB.
    // (Dividing by N / 2, as before, read every tone 6 dB low.)
    const float normalisation = 4.0f / static_cast<float>(fftSize);

    // Convert to dB with peak-hold + decay smoothing
    for (int i = 0; i < numBins; ++i)
    {
        float magnitude = fftData[static_cast<size_t>(i)] * normalisation;
        float db = magnitude > 1.0e-5f ? 20.0f * std::log10(magnitude) : -100.0f;

        db = std::max(-100.0f, db);

        if (db > smoothed[static_cast<size_t>(i)])
            smoothed[static_cast<size_t>(i)] = db; // instant attack
        else
            smoothed[static_cast<size_t>(i)] = smoothed[static_cast<size_t>(i)] * decayRate
                                              + db * (1.0f - decayRate);
    }
}

void SpectrumDSP::applyMovingAverage(const float* in, float* out, int numBins, int windowSize)
{
    int half = windowSize / 2;
    for (int i = 0; i < numBins; ++i)
    {
        float sum = 0.0f;
        int count = 0;
        for (int k = -half; k <= half; ++k)
        {
            int idx = i + k;
            if (idx >= 0 && idx < numBins)
            {
                sum += in[idx];
                ++count;
            }
        }
        // Peak-preserving: fills the valleys between bins but never lowers a bin, so a
        // tone still reads its level (a plain dB average read a pure tone ~4 dB low)
        out[i] = juce::jmax(in[i], sum / (float) count);
    }
}
