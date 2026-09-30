#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Common/MeterModule.h"
#include "../Common/EasingUtils.h"
#include "../../Core/AudioFifo.h"
#include "../../Core/LoudnessTarget.h"
#include "../../DSP/LufsDSP.h"

class LufsMeterModule : public MeterModule, public juce::Timer
{
public:
    // onResetRequested clears the measurement on the audio thread (the processor's resetLoudnessSession)
    LufsMeterModule(AudioFifo<LufsMeterData>& fifoToUse, std::function<void()> onResetRequested,
                    juce::AudioProcessorValueTreeState* apvts = nullptr);
    ~LufsMeterModule() override;

    void paintModule(juce::Graphics& g) override;
    void resizedModule() override;
    
    void timerCallback() override;
    void mouseDoubleClick(const juce::MouseEvent&) override;

    // Clears Integrated, LRA and the session maxima (RESET button, double-click)
    void resetMeasurement();

    const ff360_labs::LoudnessTarget& getCurrentTarget() const { return currentTargetProfile; }

private:
    AudioFifo<LufsMeterData>& meterFifo;
    std::function<void()> onReset;
    LufsMeterData currentData;
    float lastDrawn[3] { 1.0f, 1.0f, 1.0f }; // damper outputs at the last repaint
    float shownLra = -1.0f, shownIntegrated = 1.0f;

    juce::ComboBox targetSelector;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> targetAttachment;
    juce::TextButton resetButton{"RESET"};

    ff360_labs::LoudnessTarget currentTargetProfile;

    ff360_labs::SpringDamper momentaryDamper { -60.0f, 180.0f, 24.0f };
    ff360_labs::SpringDamper shortTermDamper { -60.0f, 140.0f, 22.0f };
    ff360_labs::SpringDamper integratedDamper{ -60.0f, 120.0f, 20.0f };

    void drawLufsDial (juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawStatCluster (juce::Graphics& g, juce::Rectangle<float> bounds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LufsMeterModule)
};
