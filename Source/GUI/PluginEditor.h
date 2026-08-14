#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
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

    juce::TextButton btnGridMode;
    juce::TextButton btnFocusMode;
    juce::TextButton btnExportReport;
    juce::TextButton btnColorblindMode;
    juce::Label perfBadge;
    juce::ComboBox layoutComboBox;
    juce::ComboBox addModuleComboBox;

    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> colorblindAttachment;

    MeterModule* createModule (MeterModuleType type);
    void populateLayoutPresets();
    void triggerExportReport();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FF360MeterEditor)
};
