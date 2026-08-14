#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>

struct SpectrumData
{
    std::vector<float> magnitudesL; // Left channel (or mono)
    std::vector<float> magnitudesR; // Right channel (empty if mono)
};

// FFT resolution options exposed as a user-facing selector (Phase 10.5)
enum class FFTResolution
{
    Low  = 10, // 1024 bins  — lower CPU, less frequency resolution
    Mid  = 11, // 2048 bins  — balanced (default)
    High = 12  // 4096 bins  — best frequency resolution, higher CPU/latency
};

class SpectrumDSP
{
public:
    SpectrumDSP();

    void prepare(double sampleRate, int samplesPerBlock);

    // Set FFT resolution. Calling this reinitialises internal state.
    void setFFTResolution(FFTResolution resolution);
    FFTResolution getFFTResolution() const { return currentResolution; }

    int getFFTSize()  const { return fftSize; }
    int getNumBins()  const { return numBins; }

    // Process block — fills outData with new frame if FFT completed this block.
    // Returns true if a new frame is available.
    bool processBlock(const juce::AudioBuffer<float>& buffer, SpectrumData& outData);

private:
    double currentSampleRate = 48000.0;
    FFTResolution currentResolution = FFTResolution::Mid;

    int fftOrder = 11;
    int fftSize  = 1 << 11; // 2048
    int numBins  = fftSize / 2;

    std::unique_ptr<juce::dsp::FFT> fft;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;

    // Separate L and R FIFOs + FFT data buffers
    std::vector<float> fifoL, fifoR;
    std::vector<float> fftDataL, fftDataR;
    std::vector<float> smoothedMagnitudesL, smoothedMagnitudesR;

    int fifoIndex = 0;
    float decayRate = 0.9f;

    void initBuffers();
    void processFFT(std::vector<float>& fftData, std::vector<float>& smoothed);
    void applyMovingAverage(std::vector<float>& data, int windowSize = 3);
};

