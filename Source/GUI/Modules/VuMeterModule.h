#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Common/MeterModule.h"
#include "../../Core/AudioFifo.h"
#include "../../DSP/VuDSP.h"
#include <vector>

class VuMeterModule : public MeterModule, public juce::Timer
{
public:
    VuMeterModule(AudioFifo<VuMeterData>& fifoToUse, VuDSP* dsp = nullptr, juce::AudioProcessorValueTreeState* apvts = nullptr);
    ~VuMeterModule() override;

    void paintModule(juce::Graphics& g) override;
    void resizedModule() override;
    
    void timerCallback() override;

private:
    AudioFifo<VuMeterData>& meterFifo;
    VuDSP* vuDSPInstance = nullptr;
    VuMeterData currentData;

    // Single source of timing truth: 1:1 direct tracking of DSP ballistics
    float renderedVuL = -60.0f;
    float renderedVuR = -60.0f;

    // Phase 9.2: Calibration Reference Level Selector
    juce::ComboBox calibrationSelector;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> calibrationAttachment;
    float currentRefLevelDb = -18.0f;

    // Phase 9.1: Dev Debug Overlay
    juce::TextButton debugButton { "DEV OSC" };
    bool showDebugOverlay = false;

    struct DebugSample
    {
        float dspVu = -20.0f;
        float needleVu = -20.0f;
        float rawDbfs = -60.0f;
    };
    static constexpr size_t DEBUG_HISTORY_SIZE = 120; // 2 seconds @ 60 FPS
    std::vector<DebugSample> debugHistory;
    size_t debugWriteIndex = 0;

    void drawVuArcGauge (juce::Graphics& g, juce::Rectangle<float> bounds, float vuValue, const juce::String& channelLabel);
    void drawDebugOverlay (juce::Graphics& g, juce::Rectangle<float> bounds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VuMeterModule)
};

