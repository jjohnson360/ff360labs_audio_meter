#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <vector>

// FFT resolution options exposed as a user-facing selector (Phase 10.5)
enum class FFTResolution
{
    Low  = 10, // 1024 bins  — lower CPU, less frequency resolution
    Mid  = 11, // 2048 bins  — balanced (default)
    High = 12  // 4096 bins  — best frequency resolution, higher CPU/latency
};

// Fixed size (sized for the largest FFT) so a frame can be queued without allocating
struct SpectrumData
{
    static constexpr int MaxBins = 1 << (static_cast<int>(FFTResolution::High) - 1); // 2048

    int numBins = 0;          // valid entries in magnitudesL / magnitudesR
    bool hasRight = false;    // false for a mono input
    double sampleRate = 48000.0;
    std::array<float, MaxBins> magnitudesL {}; // dBFS per bin, Left channel (or mono)
    std::array<float, MaxBins> magnitudesR {}; // dBFS per bin, Right channel
};

class SpectrumDSP
{
public:
    SpectrumDSP();

    void prepare(double sampleRate, int samplesPerBlock);

    // Thread-safe: may be called from the UI thread. The audio thread switches
    // resolution at the start of its next block, without allocating.
    void setFFTResolution(FFTResolution resolution);
    FFTResolution getFFTResolution() const { return static_cast<FFTResolution>(requestedOrder.load()); }

    int getFFTSize()  const { return 1 << activeOrder; }
    int getNumBins()  const { return (1 << activeOrder) / 2; }

    // Process block — fills outData with new frame if FFT completed this block.
    // Returns true if a new frame is available.
    bool processBlock(const juce::AudioBuffer<float>& buffer, SpectrumData& outData);

private:
    static constexpr int MinOrder = static_cast<int>(FFTResolution::Low);
    static constexpr int MaxOrder = static_cast<int>(FFTResolution::High);
    static constexpr int NumOrders = MaxOrder - MinOrder + 1;
    static constexpr int MaxFFTSize = 1 << MaxOrder;

    double currentSampleRate = 48000.0;

    std::atomic<int> requestedOrder { static_cast<int>(FFTResolution::Mid) };
    int activeOrder = static_cast<int>(FFTResolution::Mid);

    // One FFT and window per resolution, built up front so switching never allocates
    std::array<std::unique_ptr<juce::dsp::FFT>, NumOrders> ffts;
    std::array<std::vector<float>, NumOrders> windows;

    std::vector<float> fifoL, fifoR;
    std::vector<float> fftData;
    std::vector<float> smoothedMagnitudesL, smoothedMagnitudesR;

    int fifoIndex = 0;
    float decayRate = 0.9f;

    void resetState();
    void updateDecayRate();
    void processFFT(const std::vector<float>& fifo, std::vector<float>& smoothed);
    static void applyMovingAverage(const float* in, float* out, int numBins, int windowSize = 3);
};
