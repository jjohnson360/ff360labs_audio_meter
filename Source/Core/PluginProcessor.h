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

    // Phase 5: Phase Scope
    PhaseScopeDSP phaseScopeDSP;
    AudioFifo<PhaseScopeData> phaseScopeFifo;

    // Phase 5: Spectrum Analyzer
    SpectrumDSP spectrumDSP;
    AudioFifo<SpectrumData> spectrumFifo;

    // Phase 5: Histogram
    HistogramDSP histogramDSP;
    AudioFifo<HistogramData> histogramFifo;
    std::atomic<bool> triggerHistogramReset { false };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FF360MeterProcessor)
};
