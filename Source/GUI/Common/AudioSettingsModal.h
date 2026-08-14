#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "../../Core/PluginProcessor.h"

class AudioSettingsModal : public juce::Component, public juce::Timer
{
public:
    AudioSettingsModal(FF360MeterProcessor& processor, juce::AudioDeviceManager* deviceManagerToUse = nullptr);
    ~AudioSettingsModal() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    static void showModal(juce::Component* parent, FF360MeterProcessor& processor, juce::AudioDeviceManager* deviceManager);

private:
    FF360MeterProcessor& audioProcessor;
    juce::AudioDeviceManager* deviceManager = nullptr;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;

    juce::TextButton btnClose { "DONE" };

    juce::String osGuidanceText;
    bool hasLoopbackDevice = false;
    juce::String detectedLoopbackName;

    void inspectPlatformLoopback();
    void renderStatusBadge(juce::Graphics& g, juce::Rectangle<float> area);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSettingsModal)
};
