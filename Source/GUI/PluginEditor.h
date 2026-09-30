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

class AudioSettingsModal;

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

    // UI zoom: the whole interface is laid out at (window size / zoom) and scaled up by
    // an affine transform, so text and meters stay sharp. Saved with the session.
    static constexpr float minZoom = 0.75f, maxZoom = 2.0f;
    void setUiZoom (float newZoom);
    float getUiZoom() const { return uiZoom; }

    // Unscaled layout size and resize limits (the minimum scales with the zoom)
    static constexpr int designWidth = 1120, designHeight = 680;
    static constexpr int minLogicalWidth = 900, minLogicalHeight = 480;

    juce::Component& getCanvas() { return canvas; }
    MeterDashboard& getDashboard() { return meterDashboard; }

private:
    // Everything is laid out on this, then scaled as a whole
    struct Canvas : public juce::Component
    {
        std::function<void (juce::Graphics&)> onPaint;
        void paint (juce::Graphics& g) override { if (onPaint) onPaint (g); }
    };

    FF360MeterProcessor& audioProcessor;
    FF360LabsLookAndFeel customLookAndFeel;

    Canvas canvas;
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

    bool colorblindModeActive = false;

    float uiZoom = 1.0f;
    bool sizeRestored = false; // don't save the interim sizes set while the constructor runs
    bool loadingLayout = false;

    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<AudioSettingsModal> settingsModal;
    std::unique_ptr<juce::AlertWindow> layoutNameAlert; // owned here so it never outlives the look and feel

    MeterModule* createModule (MeterModuleType type);
    void paintHeader (juce::Graphics& g);
    void layoutCanvas();
    void storeActiveLayout();
    void populateLayoutPresets();
    void updateInputDeviceList();
    void triggerExportReport(bool csvMode);
    void openAudioSettings();
    void showSettingsMenu();
    void showAboutDialog();
    juce::AudioDeviceManager* getStandaloneDeviceManager();

    juce::TooltipWindow tooltipWindow { this, 700 }; // last: uses the look and feel above

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FF360MeterEditor)
};
