#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Common/MeterModule.h"
#include "../../Core/AudioFifo.h"
#include "../../DSP/HistogramDSP.h"

class HistogramModule : public MeterModule, public juce::Timer
{
public:
    HistogramModule(AudioFifo<HistogramData>& fifoToUse, std::function<void()> onResetCallback);
    ~HistogramModule() override;

    void paintModule(juce::Graphics& g) override;
    void resizedModule() override;
    
    void timerCallback() override;

private:
    AudioFifo<HistogramData>& meterFifo;
    std::function<void()> onReset;
    HistogramData currentData;
    HistogramData snapshotA;
    bool hasSnapshotA = false;
    bool isComparing = false;

    juce::TextButton btnReset { "RESET" };
    juce::TextButton btnCaptureA { "CAP A" };
    juce::TextButton btnToggleAB { "A/B: OFF" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HistogramModule)
};
