cmake_minimum_required(VERSION 3.22)

project(FF360Meter VERSION 1.3.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# macOS Release & Guardrails (see the ff360 Labs Global Plugin CI/CD Rules)
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum OS X deployment version" FORCE)
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary architectures" FORCE)
    endif()
endif()

# Install each plugin into the system plugin folder after it builds (on locally, off in CI)
if(DEFINED ENV{CI})
    set(FF360_COPY_DEFAULT OFF)
else()
    set(FF360_COPY_DEFAULT ON)
endif()
option(FF360_COPY_PLUGINS_AFTER_BUILD "Copy the built plugins into the system plugin folders" ${FF360_COPY_DEFAULT})

include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(juce)

# Brand fonts, as in Niche360: Barlow Condensed for chrome, JetBrains Mono for readouts
# (both SIL OFL 1.1 - see Source/GUI/LookAndFeel/Fonts/*-OFL.txt)
juce_add_binary_data(ff360_meter_fonts
    HEADER_NAME MeterFonts.h
    NAMESPACE MeterFonts
    SOURCES
        Source/GUI/LookAndFeel/Fonts/BarlowCondensed-Regular.ttf
        Source/GUI/LookAndFeel/Fonts/BarlowCondensed-Medium.ttf
        Source/GUI/LookAndFeel/Fonts/BarlowCondensed-SemiBold.ttf
        Source/GUI/LookAndFeel/Fonts/JetBrainsMono-Regular.ttf
        Source/GUI/LookAndFeel/Fonts/JetBrainsMono-Medium.ttf
)

juce_add_plugin(FF360Meter
    COMPANY_NAME "ff360 Labs"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD ${FF360_COPY_PLUGINS_AFTER_BUILD}
    # Kept as shipped since v1.0: changing either code makes hosts treat it as a different plugin
    PLUGIN_MANUFACTURER_CODE F360
    PLUGIN_CODE F301
    FORMATS VST3 Standalone AU
    PRODUCT_NAME "FF360Meter"
    BUNDLE_ID com.ff360labs.audiometer
    VST3_CATEGORIES Fx Analyzer
)

set(METER_SOURCES
    Source/Core/PluginProcessor.h
    Source/Core/PluginProcessor.cpp
    Source/Core/Constants.h
    Source/Core/AudioFifo.h
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

set(METER_DEFINITIONS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

set(METER_LIBRARIES
    ff360_meter_fonts
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_dsp
)

target_sources(FF360Meter PRIVATE ${METER_SOURCES})

target_compile_definitions(FF360Meter PUBLIC ${METER_DEFINITIONS})

target_compile_options(FF360Meter
    PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

target_link_libraries(FF360Meter
    PRIVATE
        ${METER_LIBRARIES}
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Processor tests: the real processor (and editor) run offline; `ctest` or run the exe directly.
# The exe also renders the editor: --screenshot <png> [scale] [layout]
juce_add_console_app(FF360Meter_ProcessorTests PRODUCT_NAME "FF360Meter Processor Tests")

target_sources(FF360Meter_ProcessorTests PRIVATE
    Tests/ProcessorTests.cpp
    ${METER_SOURCES}
)

target_compile_definitions(FF360Meter_ProcessorTests PRIVATE
    ${METER_DEFINITIONS}
    JucePlugin_Name="FF360Meter"
    JucePlugin_VersionString="${PROJECT_VERSION}"
    JucePlugin_Build_Standalone=0
    JUCE_LOG_ASSERTIONS=1   # the tests print assertion failures and leaks (see main)
)

target_compile_options(FF360Meter_ProcessorTests
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

target_link_libraries(FF360Meter_ProcessorTests
    PRIVATE
        ${METER_LIBRARIES}
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

enable_testing()
add_test(NAME ProcessorTests COMMAND FF360Meter_ProcessorTests)
