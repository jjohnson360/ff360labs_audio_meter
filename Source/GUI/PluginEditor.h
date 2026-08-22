#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "../Core/PluginProcessor.h"
#include "LookAndFeel/FF360LabsLookAndFeel.h"
#include "Layout/MeterDashboard.h"
#include "Layout/LayoutManager.h"
#include "Modules/PeakRmsMeterModule.h"
#include "Modules/VuMeterModule.h"
#include "Modules/LufsMeterModule.h"
#include "Modules/SpectrumAnalyzerModule.h"
#include "Modules/HistogramModule.h"
#include "Modules/PhaseScopeModule.h"
#include "Modules/PlaceholderModule.h"

class FF360MeterEditor  : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    FF360MeterEditor (FF360MeterProcessor&);
    ~FF360MeterEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    void loadLayout (const ff360_labs::DashboardLayout& layout);
    void saveCurrentLayout (const juce::String& name);
    ff360_labs::DashboardLayout getCurrentDashboardLayout() const;

private:
    FF360MeterProcessor& audioProcessor;
    FF360LabsLookAndFeel customLookAndFeel;

    MeterDashboard meterDashboard;
    juce::OwnedArray<MeterModule> dynamicModules;

    // Nav bar — consolidated & diagnostic (Phase 10.1 & 11.1)
    juce::TextButton btnDevOsc { "DEV OSC" };
    juce::ComboBox   inputDeviceComboBox;
    juce::ComboBox   addModuleComboBox;
    juce::ComboBox   layoutComboBox;
    juce::TextButton btnSettings { juce::CharPointer_UTF8("\xe2\x9a\x99") }; // ⚙ gear
    juce::Label      ioStatusDot;   // minimal ● dot (colour encodes state)
    juce::Label      perfDot;       // minimal ● dot (colour encodes perf state)

    // APVTS attachment for colorblind mode (still needed even though button is in menu)
    bool colorblindModeActive = false;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> colorblindAttachment;

    std::unique_ptr<juce::FileChooser> fileChooser;

    MeterModule* createModule (MeterModuleType type);
    void populateLayoutPresets();
    void updateInputDeviceList();
    void triggerExportReport(bool csvMode);
    void openAudioSettings();
    void showSettingsMenu();
    void showAboutDialog();
    juce::AudioDeviceManager* getStandaloneDeviceManager();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FF360MeterEditor)
};

