cmake_minimum_required(VERSION 3.22)

project(FF360Meter VERSION 1.2.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(juce)

juce_add_plugin(FF360Meter
    COMPANY_NAME "ff360 Labs"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE F360
    PLUGIN_CODE F301
    FORMATS VST3 Standalone AU
    PRODUCT_NAME "FF360Meter"
    BUNDLE_ID com.ff360labs.audiometer
)

target_sources(FF360Meter
    PRIVATE
        Source/Core/PluginProcessor.h
        Source/Core/PluginProcessor.cpp
        Source/Core/Constants.h
        Source/Core/LoudnessTarget.h
        Source/Core/SessionReport.h
        Source/Core/SessionReport.cpp
        Source/GUI/Common/EasingUtils.h
        Source/DSP/PeakRmsDSP.h
        Source/DSP/PeakRmsDSP.cpp
        Source/DSP/VuDSP.h
        Source/DSP/VuDSP.cpp
        Source/DSP/LufsDSP.h
        Source/DSP/LufsDSP.cpp
        Source/DSP/PhaseScopeDSP.h
        Source/DSP/PhaseScopeDSP.cpp
        Source/DSP/SpectrumDSP.h
        Source/DSP/SpectrumDSP.cpp
        Source/DSP/HistogramDSP.h
        Source/DSP/HistogramDSP.cpp
        Source/DSP/CalibrationTest.h
        Source/GUI/PluginEditor.h
        Source/GUI/PluginEditor.cpp
        Source/GUI/LookAndFeel/FF360LabsLookAndFeel.h
        Source/GUI/LookAndFeel/FF360LabsLookAndFeel.cpp
        Source/GUI/Common/MeterModule.h
        Source/GUI/Common/MeterModule.cpp
        Source/GUI/Common/AudioSettingsModal.h
        Source/GUI/Common/AudioSettingsModal.cpp
        Source/GUI/Layout/MeterDashboard.h
        Source/GUI/Layout/MeterDashboard.cpp
        Source/GUI/Layout/LayoutManager.h
        Source/GUI/Layout/FloatingModuleWindow.h
        Source/GUI/Modules/PeakRmsMeterModule.h
        Source/GUI/Modules/PeakRmsMeterModule.cpp
        Source/GUI/Modules/VuMeterModule.h
        Source/GUI/Modules/VuMeterModule.cpp
        Source/GUI/Modules/LufsMeterModule.h
        Source/GUI/Modules/LufsMeterModule.cpp
        Source/GUI/Modules/PhaseScopeModule.h
        Source/GUI/Modules/PhaseScopeModule.cpp
        Source/GUI/Modules/SpectrumAnalyzerModule.h
        Source/GUI/Modules/SpectrumAnalyzerModule.cpp
        Source/GUI/Modules/HistogramModule.h
        Source/GUI/Modules/HistogramModule.cpp
        Source/GUI/Modules/PlaceholderModule.h
        Source/GUI/Modules/PlaceholderModule.cpp
)

target_compile_definitions(FF360Meter
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(FF360Meter
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_devices
        juce::juce_audio_processors
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
