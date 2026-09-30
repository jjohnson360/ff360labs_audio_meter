#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "AudioFifo.h"
#include "../DSP/PeakRmsDSP.h"
#include "../DSP/VuDSP.h"
#include "../DSP/LufsDSP.h"
#include "../DSP/PhaseScopeDSP.h"
#include "../DSP/SpectrumDSP.h"
#include "../DSP/HistogramDSP.h"
class FF360MeterProcessor  : public juce::AudioProcessor
{
public:
    FF360MeterProcessor();
    ~FF360MeterProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override {}

    void resetHistogram();

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    
    // Phase 3: DSP and FIFO
    PeakRmsDSP peakRmsDSP;
    AudioFifo<MeterData> meterFifo;

    // Phase 5: VU Meter
    VuDSP vuDSP;
    AudioFifo<VuMeterData> vuFifo;

    // Phase 5: LUFS Meter
    LufsDSP lufsDSP;
    AudioFifo<LufsMeterData> lufsFifo;

    // Phase 5: Phase Scope (a frame per block; the UI drains them all at 60 Hz)
    PhaseScopeDSP phaseScopeDSP;
    AudioFifo<PhaseScopeData, 128> phaseScopeFifo;

    // Phase 5: Spectrum Analyzer (a frame per FFT, ~12-47 per second)
    SpectrumDSP spectrumDSP;
    AudioFifo<SpectrumData, 8> spectrumFifo;

    // Phase 5: Histogram (a frame per 100 ms)
    HistogramDSP histogramDSP;
    AudioFifo<HistogramData, 32> histogramFifo;
    std::atomic<bool> triggerHistogramReset { false };

    // Clears Integrated / LRA, the session maxima and the histogram. Thread-safe:
    // the audio thread performs the reset at the start of its next block.
    void resetLoudnessSession();

    // Highest sample peak per channel since the last loudness reset, in dBFS
    float getSessionPeakDb (int channel) const { return sessionPeakDb[channel == 0 ? 0 : 1].load(); }

    // Phase 9: Signal & Connection State Monitoring
    bool getIsInputConnected() const { return isInputConnected.load(); }
    bool getIsAudioSilent() const { return isAudioSilent.load(); }
    float getCurrentPeakLevelDb() const { return currentPeakLevelDb.load(); }

    // Phase 11: DEV OSC Calibrated Reference Generator
    bool isDevOscEnabled() const { return devOscEnabled.load(); }
    void setDevOscEnabled(bool enabled) { devOscEnabled.store(enabled); }

private:
    std::atomic<float>* vuRefLevelParam = nullptr;

    // Scratch frames, reused every block (kept off the audio thread's stack)
    PhaseScopeData phaseScratch;
    SpectrumData spectrumScratch;
    HistogramData histogramScratch;

    std::array<std::atomic<float>, 2> sessionPeakDb { -100.0f, -100.0f };
    std::atomic<bool> sessionPeakResetRequested { false };

    std::atomic<bool> isInputConnected { false };
    std::atomic<bool> isAudioSilent { true };
    std::atomic<float> currentPeakLevelDb { -100.0f };
    std::atomic<bool> devOscEnabled { false };
    double oscPhase = 0.0;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FF360MeterProcessor)
};
