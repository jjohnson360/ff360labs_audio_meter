#include "PluginEditor.h"
#include "../Core/Constants.h"
#include "../Core/SessionReport.h"
#include "Common/AudioSettingsModal.h"

#if JucePlugin_Build_Standalone
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

namespace
{
    constexpr int headerHeight = 40;
}

FF360MeterEditor::FF360MeterEditor (FF360MeterProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel(&customLookAndFeel);
    setOpaque(true);

    canvas.onPaint = [this] (juce::Graphics& g) { paintHeader(g); };
    canvas.setOpaque(true);
    addAndMakeVisible(canvas);

    canvas.addAndMakeVisible(meterDashboard);
    meterDashboard.onLayoutChanged = [this] { storeActiveLayout(); };

    // --- Settings button ---
    btnSettings.setTooltip("Settings: audio I/O, report export, accessible palette, grid or focus view, UI size and more.");
    btnSettings.onClick = [this] { showSettingsMenu(); };
    canvas.addAndMakeVisible(btnSettings);

    // --- Minimal status dots ---
    ioStatusDot.setText(juce::CharPointer_UTF8("\xe2\x97\x8f"), juce::dontSendNotification); // ●
    ioStatusDot.setFont(FF360LabsLookAndFeel::getCustomFont(11.0f, juce::Font::bold));
    ioStatusDot.setColour(juce::Label::textColourId, juce::Colour(0xff00e5ff));
    ioStatusDot.setJustificationType(juce::Justification::centred);
    ioStatusDot.setTooltip("I/O Status: Live Input");
    canvas.addAndMakeVisible(ioStatusDot);

    perfDot.setText(juce::CharPointer_UTF8("\xe2\x97\x8f"), juce::dontSendNotification); // ●
    perfDot.setFont(FF360LabsLookAndFeel::getCustomFont(11.0f, juce::Font::bold));
    perfDot.setColour(juce::Label::textColourId, ff360_labs::AccentGold);
    perfDot.setJustificationType(juce::Justification::centred);
    perfDot.setTooltip("Perf: 60 FPS");
    canvas.addAndMakeVisible(perfDot);

    // --- Colorblind mode (toggle lives in the Settings menu; read the saved state) ---
    if (auto* param = audioProcessor.apvts.getParameter("colorblindMode"))
    {
        colorblindModeActive = (param->getValue() > 0.5f);
        FF360LabsLookAndFeel::setColorblindModeActive(colorblindModeActive);
    }

    // --- DEV OSC Button (Phase 11.1) ---
    btnDevOsc.setClickingTogglesState(true);
    btnDevOsc.setTooltip("DEV OSC: replaces the input with a calibrated 1 kHz sine peaking at -18 dBFS, to check "
                         "the meters: Peak -18.0 dBFS, RMS -21.0 dBFS, -3.0 VU at the -18 dBFS reference, -18.0 LUFS.");
    btnDevOsc.setColour(juce::TextButton::buttonColourId, ff360_labs::ContainerDark);
    btnDevOsc.setColour(juce::TextButton::buttonOnColourId, ff360_labs::AccentGold.withAlpha(0.35f));
    btnDevOsc.setColour(juce::TextButton::textColourOffId, ff360_labs::AccentGold);
    btnDevOsc.setColour(juce::TextButton::textColourOnId, ff360_labs::TextOffWhite);
    btnDevOsc.onClick = [this] {
        bool active = btnDevOsc.getToggleState();
        audioProcessor.setDevOscEnabled(active);
        repaint();
    };
    canvas.addAndMakeVisible(btnDevOsc);

    // --- Audio Input Device Selector (Phase 11.1) ---
    updateInputDeviceList();
    canvas.addAndMakeVisible(inputDeviceComboBox);

    // --- Add Module combo ---
    addModuleComboBox.setTextWhenNothingSelected("+ Add Module");
    addModuleComboBox.setTooltip("Add a meter module to the dashboard.");
    addModuleComboBox.addItem("Peak / RMS Meter", 1);
    addModuleComboBox.addItem("VU Meter", 2);
    addModuleComboBox.addItem("LUFS Meter", 3);
    addModuleComboBox.addItem("Spectrum Analyzer", 4);
    addModuleComboBox.addItem("Histogram (5 Min)", 5);
    addModuleComboBox.addItem("Phase Scope", 6);
    canvas.addAndMakeVisible(addModuleComboBox);

    addModuleComboBox.onChange = [this] {
        int selectedId = addModuleComboBox.getSelectedId();
        if (selectedId > 0)
        {
            MeterModuleType type = MeterModuleType::Unknown;
            if (selectedId == 1) type = MeterModuleType::PeakRms;
            else if (selectedId == 2) type = MeterModuleType::VU;
            else if (selectedId == 3) type = MeterModuleType::LUFS;
            else if (selectedId == 4) type = MeterModuleType::Spectrum;
            else if (selectedId == 5) type = MeterModuleType::Histogram;
            else if (selectedId == 6) type = MeterModuleType::PhaseScope;

            auto* newModule = createModule(type);
            if (newModule != nullptr)
            {
                dynamicModules.add(newModule);
                meterDashboard.addModule(newModule);
            }

            addModuleComboBox.setSelectedId(0, juce::dontSendNotification);
        }
    };

    layoutComboBox.setTooltip("Layouts: factory module sets, your saved layouts, or save the current dashboard as a new one.");
    populateLayoutPresets();
    canvas.addAndMakeVisible(layoutComboBox);

    // Check if a saved active layout exists in APVTS state
    auto activeLayoutTree = audioProcessor.apvts.state.getChildWithName("ActiveLayout");
    if (activeLayoutTree.isValid())
    {
        loadLayout(ff360_labs::DashboardLayout::fromValueTree(activeLayoutTree));
    }
    else
    {
        // Default factory layout: Mastering
        const auto& factory = ff360_labs::DashboardLayout::getFactoryPresets();
        loadLayout(factory[0]);
    }

    startTimerHz(4); // 4Hz performance budget and I/O status monitor

    // Read the saved zoom and size before anything can resize the editor: setResizeLimits()
    // clamps the still-empty editor to the minimum size, and resized() would otherwise save that.
    const auto& state = audioProcessor.apvts.state;
    uiZoom = juce::jlimit(minZoom, maxZoom, (float) state.getProperty("uiZoom", 1.0f));
    const int savedW = (int) state.getProperty("editorWidth",  juce::roundToInt(designWidth  * uiZoom));
    const int savedH = (int) state.getProperty("editorHeight", juce::roundToInt(designHeight * uiZoom));

    setResizable(true, true);
    setResizeLimits(juce::roundToInt(minLogicalWidth * uiZoom), juce::roundToInt(minLogicalHeight * uiZoom), 7680, 4320);
    setSize(juce::jmax(savedW, juce::roundToInt(minLogicalWidth * uiZoom)),
            juce::jmax(savedH, juce::roundToInt(minLogicalHeight * uiZoom)));
    sizeRestored = true;
}

FF360MeterEditor::~FF360MeterEditor()
{
    stopTimer();
    settingsModal.reset();
    layoutNameAlert.reset();

    // Save current active layout before closing
    storeActiveLayout();

    // Close detached windows before dynamicModules deletes the modules they host
    meterDashboard.onLayoutChanged = nullptr;
    meterDashboard.clearAllModules();

    setLookAndFeel(nullptr);
}

void FF360MeterEditor::storeActiveLayout()
{
    if (loadingLayout)
        return;

    auto& state = audioProcessor.apvts.state;
    auto existingActive = state.getChildWithName("ActiveLayout");
    if (existingActive.isValid())
        state.removeChild(existingActive, nullptr);

    state.addChild(getCurrentDashboardLayout().toValueTree("ActiveLayout"), -1, nullptr);
}

void FF360MeterEditor::timerCallback()
{
    // Adaptive Frame Rate / CPU Budget Monitor
    int activeCount = dynamicModules.size();
    float multiplier = (activeCount > 6) ? 0.75f : 1.0f;

    for (auto* m : dynamicModules)
    {
        if (m != nullptr)
            m->setThrottleMultiplier(multiplier);
    }

    // Perf dot: gold = 60 FPS, amber = throttled
    if (multiplier < 1.0f)
    {
        perfDot.setColour(juce::Label::textColourId, FF360LabsLookAndFeel::getWarningColour());
        perfDot.setTooltip("Perf: ~45 FPS (throttled \xe2\x80\x94 >6 modules active)");
    }
    else
    {
        perfDot.setColour(juce::Label::textColourId, ff360_labs::AccentGold);
        perfDot.setTooltip("Perf: 60 FPS");
    }

    // Phase 11.1: I/O dot & DEV OSC state
    bool isOsc        = audioProcessor.isDevOscEnabled();
    bool isConnected  = audioProcessor.getIsInputConnected();
    bool isSilent     = audioProcessor.getIsAudioSilent();
    float peakDb      = audioProcessor.getCurrentPeakLevelDb();

    btnDevOsc.setToggleState(isOsc, juce::dontSendNotification);

    if (isOsc)
    {
        ioStatusDot.setColour(juce::Label::textColourId, juce::Colour(0xffd946ef));
        ioStatusDot.setTooltip("DEV OSC active: metering the internal 1 kHz sine at -18 dBFS.");
    }
    else if (!isConnected)
    {
        ioStatusDot.setColour(juce::Label::textColourId, ff360_labs::AccentAmberRed);
        ioStatusDot.setTooltip("No input: the host or device isn't sending audio to the meter.");
    }
    else if (isSilent)
    {
        ioStatusDot.setColour(juce::Label::textColourId, ff360_labs::AccentGold.withAlpha(0.8f));
        ioStatusDot.setTooltip("Input connected, but silent (below about -80 dBFS).");
    }
    else
    {
        ioStatusDot.setColour(juce::Label::textColourId, juce::Colour(0xff00e5ff));
        ioStatusDot.setTooltip("Live audio: peak " + juce::String(peakDb, 1) + " dBFS in the last block.");
    }
}

MeterModule* FF360MeterEditor::createModule (MeterModuleType type)
{
    switch (type)
    {
        case MeterModuleType::PeakRms:
            return new PeakRmsMeterModule (audioProcessor.meterFifo);
        case MeterModuleType::VU:
            return new VuMeterModule (audioProcessor.vuFifo, &audioProcessor.vuDSP, &audioProcessor.apvts);
        case MeterModuleType::LUFS:
            return new LufsMeterModule (audioProcessor.lufsFifo, [this] { audioProcessor.resetLoudnessSession(); },
                                        &audioProcessor.apvts);
        case MeterModuleType::Spectrum:
            return new SpectrumAnalyzerModule (audioProcessor.spectrumFifo, audioProcessor.spectrumDSP, audioProcessor.apvts);
        case MeterModuleType::Histogram:
            return new HistogramModule (audioProcessor.histogramFifo, [this] { audioProcessor.resetHistogram(); });
        case MeterModuleType::PhaseScope:
            return new PhaseScopeModule (audioProcessor.phaseScopeFifo);
        default:
            return nullptr;
    }
}

void FF360MeterEditor::setUiZoom (float newZoom)
{
    newZoom = juce::jlimit(minZoom, maxZoom, newZoom);

    // Keep the same logical layout: the window grows or shrinks with the zoom
    const float logicalW = (float) getWidth()  / uiZoom;
    const float logicalH = (float) getHeight() / uiZoom;

    uiZoom = newZoom;
    audioProcessor.apvts.state.setProperty("uiZoom", uiZoom, nullptr);
    meterDashboard.setUiZoom(uiZoom);

    setResizeLimits(juce::roundToInt(minLogicalWidth * uiZoom), juce::roundToInt(minLogicalHeight * uiZoom), 7680, 4320);
    setSize(juce::roundToInt(logicalW * uiZoom), juce::roundToInt(logicalH * uiZoom));
    resized();
}

void FF360MeterEditor::showSettingsMenu()
{
    juce::PopupMenu menu;

    // --- Audio I/O ---
    menu.addItem(1, "Audio I/O Settings...", getStandaloneDeviceManager() != nullptr);

    menu.addSeparator();

    // --- Export ---
    juce::PopupMenu exportSub;
    exportSub.addItem(10, "Export Branded Report (HTML)...");
    exportSub.addItem(11, "Export Spreadsheet (CSV)...");
    menu.addSubMenu("Export Report", exportSub);

    menu.addSeparator();

    // --- Accessibility ---
    menu.addItem(20, "Accessible Palette", true, colorblindModeActive);

    menu.addSeparator();

    // --- Layout Mode ---
    bool isGrid = (meterDashboard.getLayoutMode() == LayoutMode::Grid);
    menu.addItem(30, "Grid Mode",    true, isGrid);
    menu.addItem(31, "Focus Mode",   true, !isGrid);

    menu.addSeparator();

    // --- UI Size (zoom; the window keeps its layout and scales) ---
    juce::PopupMenu sizeSub;
    const float zooms[] = { 0.75f, 1.00f, 1.25f, 1.50f, 1.75f, 2.00f };
    for (int i = 0; i < (int) std::size(zooms); ++i)
        sizeSub.addItem(40 + i, juce::String(juce::roundToInt(zooms[i] * 100.0f)) + "%", true,
                        std::abs(zooms[i] - uiZoom) < 0.01f);
    menu.addSubMenu("UI Size", sizeSub);

    // --- Full Screen (standalone only) ---
   #if JucePlugin_Build_Standalone
    menu.addSeparator();
    menu.addItem(50, "Full Screen");
   #endif

    menu.addSeparator();
    menu.addItem(99, "About ff360_labs Meter...");

    auto options = juce::PopupMenu::Options()
                       .withTargetComponent(&btnSettings)
                       .withMaximumNumColumns(1);

    menu.showMenuAsync(options, [this, zooms](int result)
    {
        if (result == 1)
        {
            openAudioSettings();
        }
        else if (result == 10)
        {
            triggerExportReport(false);
        }
        else if (result == 11)
        {
            triggerExportReport(true);
        }
        else if (result == 20)
        {
            // Toggle colorblind mode
            colorblindModeActive = !colorblindModeActive;
            FF360LabsLookAndFeel::setColorblindModeActive(colorblindModeActive);
            if (auto* param = audioProcessor.apvts.getParameter("colorblindMode"))
                param->setValueNotifyingHost(colorblindModeActive ? 1.0f : 0.0f);
            repaint();
        }
        else if (result == 30)
        {
            meterDashboard.setLayoutMode(LayoutMode::Grid);
        }
        else if (result == 31)
        {
            meterDashboard.setLayoutMode(LayoutMode::Maximized);
        }
        else if (result >= 40 && result < 40 + (int) std::size(zooms))
        {
            setUiZoom(zooms[result - 40]);
        }
       #if JucePlugin_Build_Standalone
        else if (result == 50)
        {
            if (auto* peer = getPeer())
                peer->setFullScreen(!peer->isFullScreen());
        }
       #endif
        else if (result == 99)
        {
            showAboutDialog();
        }
    });
}

void FF360MeterEditor::showAboutDialog()
{
    juce::String version = juce::String("Beta v") + JucePlugin_VersionString
                         + " (" + juce::String(__DATE__) + ")";

    juce::String msg = "ff360_labs Modular Audio Metering Plugin\n\n"
                     + version + "\n\n"
                     "Built with JUCE. Barlow Condensed and JetBrains Mono\n"
                     "are used under the SIL Open Font License 1.1.\n\n"
                     "(c) ff360_labs";

    juce::AlertWindow::showAsync(juce::MessageBoxOptions()
                                     .withIconType(juce::MessageBoxIconType::InfoIcon)
                                     .withTitle("About ff360_labs Meter")
                                     .withMessage(msg)
                                     .withButton("Close")
                                     .withAssociatedComponent(this),
                                 nullptr);
}

void FF360MeterEditor::openAudioSettings()
{
    if (settingsModal != nullptr)
        return;

    settingsModal = std::make_unique<AudioSettingsModal>(audioProcessor, getStandaloneDeviceManager());
    settingsModal->onClose = [safeThis = juce::Component::SafePointer<FF360MeterEditor>(this)]
    {
        // Deleted asynchronously: this runs inside the modal's own button callback
        juce::MessageManager::callAsync([safeThis]
        {
            if (safeThis != nullptr)
            {
                safeThis->settingsModal.reset();
                safeThis->updateInputDeviceList();
            }
        });
    };
    canvas.addAndMakeVisible(*settingsModal);
    settingsModal->setBounds(canvas.getLocalBounds());
}

juce::AudioDeviceManager* FF360MeterEditor::getStandaloneDeviceManager()
{
   #if JucePlugin_Build_Standalone
    if (auto* holder = juce::StandalonePluginHolder::getInstance())
        return &holder->deviceManager;
   #endif
    return nullptr;
}

void FF360MeterEditor::triggerExportReport(bool csvMode)
{
    int targetIdx = 0;
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(audioProcessor.apvts.getParameter("targetProfile")))
        targetIdx = choice->getIndex();
    auto targetProfile = ff360_labs::LoudnessTarget::getPresetByIndex(targetIdx);

    const auto& lufs = audioProcessor.lufsDSP;
    auto data = ff360_labs::SessionReportData::collect(targetProfile,
                                                       lufs.getIntegrated(), lufs.getLRA(),
                                                       lufs.getMaxShortTerm(), lufs.getMaxMomentary(),
                                                       audioProcessor.getSessionPeakDb(0),
                                                       audioProcessor.getSessionPeakDb(1));

    if (!csvMode)
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Save Mastering Report",
            juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("ff360labs_session_report.html"),
            "*.html");
        auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                   | juce::FileBrowserComponent::warnAboutOverwriting;
        fileChooser->launchAsync(flags, [data](const juce::FileChooser& fc) {
            auto file = fc.getResult();
            if (file != juce::File{})
                data.exportHtml(file);
        });
    }
    else
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Save Loudness CSV Data",
            juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("ff360labs_session_data.csv"),
            "*.csv");
        auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                   | juce::FileBrowserComponent::warnAboutOverwriting;
        fileChooser->launchAsync(flags, [data](const juce::FileChooser& fc) {
            auto file = fc.getResult();
            if (file != juce::File{})
                data.exportCsv(file);
        });
    }
}

void FF360MeterEditor::populateLayoutPresets()
{
    layoutComboBox.clear(juce::dontSendNotification);
    layoutComboBox.setTextWhenNothingSelected("Layouts");

    const auto& factory = ff360_labs::DashboardLayout::getFactoryPresets();
    for (size_t i = 0; i < factory.size(); ++i)
        layoutComboBox.addItem("Layout: " + factory[i].name, (int)i + 1);

    auto userLayoutsTree = audioProcessor.apvts.state.getChildWithName("UserLayouts");
    if (userLayoutsTree.isValid() && userLayoutsTree.getNumChildren() > 0)
    {
        layoutComboBox.addSeparator();
        for (int i = 0; i < userLayoutsTree.getNumChildren(); ++i)
        {
            auto child = userLayoutsTree.getChild(i);
            layoutComboBox.addItem("Custom: " + child.getProperty("name", "User Layout").toString(), 100 + i);
        }
    }

    layoutComboBox.addSeparator();
    layoutComboBox.addItem("+ Save Current Layout...", 999);

    layoutComboBox.onChange = [this] {
        int id = layoutComboBox.getSelectedId();
        if (id >= 1 && id <= 4)
        {
            const auto& f = ff360_labs::DashboardLayout::getFactoryPresets();
            loadLayout(f[(size_t)(id - 1)]);
        }
        else if (id >= 100 && id < 900)
        {
            auto userLayoutsTree = audioProcessor.apvts.state.getChildWithName("UserLayouts");
            int userIdx = id - 100;
            if (userLayoutsTree.isValid() && userIdx < userLayoutsTree.getNumChildren())
                loadLayout(ff360_labs::DashboardLayout::fromValueTree(userLayoutsTree.getChild(userIdx)));
        }
        else if (id == 999)
        {
            if (layoutNameAlert != nullptr)
                return;

            layoutNameAlert = std::make_unique<juce::AlertWindow>("Save Custom Layout", "Enter a name for the current dashboard layout:",
                                                                  juce::MessageBoxIconType::QuestionIcon, this);
            layoutNameAlert->setLookAndFeel(&customLookAndFeel);
            layoutNameAlert->addTextEditor("layoutName", "Custom Layout", "Layout Name:");
            layoutNameAlert->addButton("Save",   1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
            layoutNameAlert->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));
            layoutNameAlert->enterModalState(true, juce::ModalCallbackFunction::create(
                [safeThis = juce::Component::SafePointer<FF360MeterEditor>(this)](int result)
            {
                if (safeThis == nullptr || safeThis->layoutNameAlert == nullptr)
                    return;

                if (result == 1)
                {
                    juce::String name = safeThis->layoutNameAlert->getTextEditorContents("layoutName").trim();
                    if (name.isNotEmpty())
                        safeThis->saveCurrentLayout(name);
                }
                safeThis->populateLayoutPresets();

                // Not deleted inside its own callback
                juce::MessageManager::callAsync([safeThis] { if (safeThis != nullptr) safeThis->layoutNameAlert.reset(); });
            }), false);
        }
    };
}

void FF360MeterEditor::loadLayout (const ff360_labs::DashboardLayout& layout)
{
    {
        const juce::ScopedValueSetter<bool> svs(loadingLayout, true);

        meterDashboard.clearAllModules();
        dynamicModules.clear();

        for (auto type : layout.moduleTypes)
        {
            auto* m = createModule(type);
            if (m != nullptr)
            {
                dynamicModules.add(m);
                meterDashboard.addModule(m);
            }
        }

        meterDashboard.setLayoutMode(layout.mode);
    }

    storeActiveLayout();
}

void FF360MeterEditor::saveCurrentLayout (const juce::String& name)
{
    auto layout = getCurrentDashboardLayout();
    layout.name = name;

    auto userLayoutsTree = audioProcessor.apvts.state.getOrCreateChildWithName("UserLayouts", nullptr);
    userLayoutsTree.addChild(layout.toValueTree(), -1, nullptr);
}

ff360_labs::DashboardLayout FF360MeterEditor::getCurrentDashboardLayout() const
{
    ff360_labs::DashboardLayout layout;
    layout.name = "Current";
    layout.mode = meterDashboard.getLayoutMode();

    for (auto* m : meterDashboard.getModules())
    {
        if (m != nullptr)
            layout.moduleTypes.push_back(m->getModuleType());
    }
    return layout;
}

void FF360MeterEditor::updateInputDeviceList()
{
    inputDeviceComboBox.clear(juce::dontSendNotification);

   #if JucePlugin_Build_Standalone
    if (auto* devMgr = getStandaloneDeviceManager())
    {
        auto* currentType = devMgr->getCurrentDeviceTypeObject();
        if (currentType != nullptr)
        {
            inputDeviceComboBox.setTooltip("Input device to meter. For system audio, choose a loopback "
                                           "input (Stereo Mix, VB-Cable, BlackHole).");
            auto devices = currentType->getDeviceNames(true); // input devices
            auto currentSetup = devMgr->getAudioDeviceSetup();
            int selectedIdx = 0;

            for (int i = 0; i < devices.size(); ++i)
            {
                juce::String devName = devices[i];
                inputDeviceComboBox.addItem("In: " + devName, i + 1);
                if (devName == currentSetup.inputDeviceName)
                    selectedIdx = i + 1;
            }

            if (selectedIdx > 0)
                inputDeviceComboBox.setSelectedId(selectedIdx, juce::dontSendNotification);
            else if (!devices.isEmpty())
                inputDeviceComboBox.setSelectedId(1, juce::dontSendNotification);
            else
                inputDeviceComboBox.setTextWhenNothingSelected("In: No Device");

            inputDeviceComboBox.onChange = [this, devMgr, currentType] {
                int id = inputDeviceComboBox.getSelectedId();
                if (id > 0 && currentType != nullptr)
                {
                    auto devs = currentType->getDeviceNames(true);
                    int idx = id - 1;
                    if (idx >= 0 && idx < devs.size())
                    {
                        auto setup = devMgr->getAudioDeviceSetup();
                        setup.inputDeviceName = devs[idx];
                        setup.useDefaultInputChannels = true;
                        devMgr->setAudioDeviceSetup(setup, true);
                    }
                }
            };
            return;
        }
    }
   #endif

    inputDeviceComboBox.setTooltip("In a plugin, the meter reads the track or bus it is inserted on.");
    inputDeviceComboBox.addItem("In: DAW Host Audio", 1);
    inputDeviceComboBox.setSelectedId(1, juce::dontSendNotification);
    inputDeviceComboBox.setEnabled(false);
}

void FF360MeterEditor::paint (juce::Graphics& g)
{
    g.fillAll(ff360_labs::BackgroundDark);
}

void FF360MeterEditor::paintHeader (juce::Graphics& g)
{
    g.fillAll(ff360_labs::BackgroundDark);

    // Header Bar
    auto headerRect = canvas.getLocalBounds().removeFromTop(headerHeight).toFloat();
    g.setColour(ff360_labs::ContainerDark);
    g.fillRect(headerRect);

    // Hairline bottom border
    g.setColour(ff360_labs::HairlineBorder);
    g.drawHorizontalLine((int)headerRect.getBottom() - 1, headerRect.getX(), headerRect.getRight());

    // Brand Title
    auto textArea = headerRect.toNearestInt().withTrimmedLeft(20);
    const auto brandFont = FF360LabsLookAndFeel::getUiFont(20.0f, juce::Font::bold);
    const juce::String brand ("ff360_labs");
    g.setFont(brandFont);
    g.setColour(ff360_labs::AccentGold);
    g.drawText(brand, textArea, juce::Justification::centredLeft, true);
    textArea.removeFromLeft(juce::roundToInt(juce::GlyphArrangement::getStringWidth(brandFont, brand)));

    // Mockup keeps the separator and subtitle dim, not bright white —
    // gold/brightness is reserved for the brand name and live data, not chrome.
    g.setColour(ff360_labs::TextMuted);
    g.setFont(FF360LabsLookAndFeel::getUiFont(16.0f).withExtraKerningFactor(0.08f));
    g.drawText("  //  MODULAR METER", textArea, juce::Justification::centredLeft, true);
}

void FF360MeterEditor::resized()
{
    // Only save sizes the user chose, not the interim ones set while the constructor runs
    if (sizeRestored)
    {
        audioProcessor.apvts.state.setProperty("editorWidth",  getWidth(),  nullptr);
        audioProcessor.apvts.state.setProperty("editorHeight", getHeight(), nullptr);
    }

    canvas.setTransform({});
    canvas.setBounds(0, 0, juce::roundToInt((float) getWidth() / uiZoom), juce::roundToInt((float) getHeight() / uiZoom));
    canvas.setTransform(juce::AffineTransform::scale(uiZoom));
    layoutCanvas();
}

void FF360MeterEditor::layoutCanvas()
{
    auto bounds = canvas.getLocalBounds();
    auto headerRect = bounds.removeFromTop(headerHeight);

    // Right-to-left: status dots, settings button, layout combo, add module combo, input device combo, DEV OSC button
    ioStatusDot.setBounds(headerRect.removeFromRight(18).reduced(0, 8));
    perfDot.setBounds(headerRect.removeFromRight(18).reduced(0, 8));
    headerRect.removeFromRight(4); // gap
    btnSettings.setBounds(headerRect.removeFromRight(34).reduced(2, 6));
    headerRect.removeFromRight(4); // gap
    layoutComboBox.setBounds(headerRect.removeFromRight(150).reduced(2, 6));
    addModuleComboBox.setBounds(headerRect.removeFromRight(130).reduced(2, 6));
    inputDeviceComboBox.setBounds(headerRect.removeFromRight(190).reduced(2, 6));
    headerRect.removeFromRight(4); // gap
    btnDevOsc.setBounds(headerRect.removeFromRight(76).reduced(2, 6));

    meterDashboard.setBounds(bounds.reduced(8));

    if (settingsModal != nullptr)
        settingsModal->setBounds(canvas.getLocalBounds());
}
