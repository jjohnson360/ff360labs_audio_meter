#include "PluginEditor.h"
#include "../Core/Constants.h"
#include "../Core/SessionReport.h"
#include "Common/AudioSettingsModal.h"

#if JucePlugin_Build_Standalone
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif

FF360MeterEditor::FF360MeterEditor (FF360MeterProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel(&customLookAndFeel);
    
    addAndMakeVisible(meterDashboard);

    // --- Settings button ---
    btnSettings.setTooltip("Settings & Options");
    btnSettings.onClick = [this] { showSettingsMenu(); };
    addAndMakeVisible(btnSettings);

    // --- Minimal status dots ---
    ioStatusDot.setText(juce::CharPointer_UTF8("\xe2\x97\x8f"), juce::dontSendNotification); // ●
    ioStatusDot.setFont(FF360LabsLookAndFeel::getCustomFont(11.0f, juce::Font::bold));
    ioStatusDot.setColour(juce::Label::textColourId, juce::Colour(0xff00e5ff));
    ioStatusDot.setJustificationType(juce::Justification::centred);
    ioStatusDot.setTooltip("I/O Status: Live Input");
    addAndMakeVisible(ioStatusDot);

    perfDot.setText(juce::CharPointer_UTF8("\xe2\x97\x8f"), juce::dontSendNotification); // ●
    perfDot.setFont(FF360LabsLookAndFeel::getCustomFont(11.0f, juce::Font::bold));
    perfDot.setColour(juce::Label::textColourId, ff360_labs::AccentGold);
    perfDot.setJustificationType(juce::Justification::centred);
    perfDot.setTooltip("Perf: 60 FPS");
    addAndMakeVisible(perfDot);

    // --- Colorblind mode (state tracked internally, toggle lives in Settings menu) ---
    colorblindModeActive = false;
    {
        // We need a temporary button as the attachment vehicle; attach & immediately discard the button
        // Actually: read initial APVTS state directly
        if (auto* param = audioProcessor.apvts.getParameter("colorblindMode"))
        {
            colorblindModeActive = (param->getValue() > 0.5f);
            FF360LabsLookAndFeel::setColorblindModeActive(colorblindModeActive);
        }
    }

    // --- DEV OSC Button (Phase 11.1) ---
    btnDevOsc.setClickingTogglesState(true);
    btnDevOsc.setTooltip("DEV OSC: Inject Calibrated 1 kHz Sine (-18 dBFS) Reference Tone");
    btnDevOsc.setColour(juce::TextButton::buttonColourId, ff360_labs::ContainerDark);
    btnDevOsc.setColour(juce::TextButton::buttonOnColourId, ff360_labs::AccentGold.withAlpha(0.35f));
    btnDevOsc.setColour(juce::TextButton::textColourOffId, ff360_labs::AccentGold);
    btnDevOsc.setColour(juce::TextButton::textColourOnId, ff360_labs::TextOffWhite);
    btnDevOsc.onClick = [this] {
        bool active = btnDevOsc.getToggleState();
        audioProcessor.setDevOscEnabled(active);
        repaint();
    };
    addAndMakeVisible(btnDevOsc);

    // --- Audio Input Device Selector (Phase 11.1) ---
    updateInputDeviceList();
    addAndMakeVisible(inputDeviceComboBox);

    // --- Add Module combo ---
    addModuleComboBox.setTextWhenNothingSelected("+ Add Module");
    addModuleComboBox.addItem("Peak / RMS Meter", 1);
    addModuleComboBox.addItem("VU Meter", 2);
    addModuleComboBox.addItem("LUFS Meter", 3);
    addModuleComboBox.addItem("Spectrum Analyzer", 4);
    addModuleComboBox.addItem("Histogram (5 Min)", 5);
    addModuleComboBox.addItem("Phase Scope", 6);
    addAndMakeVisible(addModuleComboBox);
    
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

    populateLayoutPresets();
    addAndMakeVisible(layoutComboBox);

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
    
    setResizable(true, true);
    setResizeLimits(900, 480, 2400, 1800);
    setSize(1120, 680);
}

FF360MeterEditor::~FF360MeterEditor()
{
    stopTimer();

    // Save current active layout before closing
    auto currentLayout = getCurrentDashboardLayout();
    auto existingActive = audioProcessor.apvts.state.getChildWithName("ActiveLayout");
    if (existingActive.isValid())
        audioProcessor.apvts.state.removeChild(existingActive, nullptr);
    
    auto newActive = currentLayout.toValueTree("ActiveLayout");
    audioProcessor.apvts.state.addChild(newActive, -1, nullptr);

    setLookAndFeel(nullptr);
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
        ioStatusDot.setTooltip("● DEV OSC ACTIVE [1 kHz @ -18 dBFS]");
    }
    else if (!isConnected)
    {
        ioStatusDot.setColour(juce::Label::textColourId, ff360_labs::AccentAmberRed);
        ioStatusDot.setTooltip("● NO INPUT DEVICE CONNECTED");
    }
    else if (isSilent)
    {
        ioStatusDot.setColour(juce::Label::textColourId, ff360_labs::AccentGold.withAlpha(0.8f));
        ioStatusDot.setTooltip("● INPUT ACTIVE // SILENT (IDLE)");
    }
    else
    {
        ioStatusDot.setColour(juce::Label::textColourId, juce::Colour(0xff00e5ff));
        ioStatusDot.setTooltip("● LIVE AUDIO ACTIVE [" + juce::String(peakDb, 1) + " dBFS]");
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
            return new LufsMeterModule (audioProcessor.lufsFifo, audioProcessor.lufsDSP, &audioProcessor.apvts);
        case MeterModuleType::Spectrum:
            return new SpectrumAnalyzerModule (audioProcessor.spectrumFifo, audioProcessor.getSampleRate());
        case MeterModuleType::Histogram:
            return new HistogramModule (audioProcessor.histogramFifo, [this] { audioProcessor.resetHistogram(); });
        case MeterModuleType::PhaseScope:
            return new PhaseScopeModule (audioProcessor.phaseScopeFifo);
        default:
            return nullptr;
    }
}

void FF360MeterEditor::showSettingsMenu()
{
    juce::PopupMenu menu;

    // --- Audio I/O ---
    menu.addItem(1, "Audio I/O Settings...");

    menu.addSeparator();

    // --- Export ---
    juce::PopupMenu exportSub;
    exportSub.addItem(10, "Export Branded Report (HTML)...");
    exportSub.addItem(11, "Export Spreadsheet (CSV)...");
    menu.addSubMenu("Export Report", exportSub);

    menu.addSeparator();

    // --- Accessibility ---
    menu.addItem(20, "Accessible Palette: " + juce::String(colorblindModeActive ? "ON" : "OFF"),
                 true, colorblindModeActive);

    menu.addSeparator();

    // --- Layout Mode ---
    bool isGrid = (meterDashboard.getLayoutMode() == LayoutMode::Grid);
    menu.addItem(30, "Grid Mode",    true, isGrid);
    menu.addItem(31, "Focus Mode",   true, !isGrid);

    menu.addSeparator();

    // --- UI Size ---
    juce::PopupMenu sizeSub;
    const std::pair<juce::String, float> sizes[] = {
        { "50%",  0.50f }, { "75%",  0.75f }, { "100%", 1.00f },
        { "125%", 1.25f }, { "150%", 1.50f }, { "175%", 1.75f }, { "200%", 2.00f }
    };
    int sizeId = 40;
    for (auto& [label, factor] : sizes)
        sizeSub.addItem(sizeId++, label);
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

    menu.showMenuAsync(options, [this](int result)
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
        else if (result >= 40 && result <= 46)
        {
            // UI Size: scale the window from the 1120x680 base size (Phase 5.7 compatible)
            const float factors[] = { 0.50f, 0.75f, 1.00f, 1.25f, 1.50f, 1.75f, 2.00f };
            float factor = factors[result - 40];
            int newW = juce::roundToInt(1120.0f * factor);
            int newH = juce::roundToInt(680.0f  * factor);
            setSize(newW, newH);
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
                     "Phases 0-10 complete.\n"
                     "Built with JUCE.\n\n"
                     "(c) ff360_labs";

    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                           "About ff360_labs Meter",
                                           msg, "Close");
}

void FF360MeterEditor::openAudioSettings()
{
    AudioSettingsModal::showModal(this, audioProcessor, getStandaloneDeviceManager(), [this] {
        updateInputDeviceList();
    });
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
    if (auto* param = audioProcessor.apvts.getParameter("targetProfile"))
        targetIdx = (int)param->getValue() * (int)(ff360_labs::LoudnessTarget::getBuiltinPresets().size() - 1);
    auto targetProfile = ff360_labs::LoudnessTarget::getPresetByIndex(targetIdx);

    float integrated = audioProcessor.lufsDSP.getIntegrated();
    float lra        = audioProcessor.lufsDSP.getLRA();
    float shortTerm  = audioProcessor.lufsDSP.getShortTerm();
    float momentary  = audioProcessor.lufsDSP.getMomentary();

    auto data = ff360_labs::SessionReportData::collect(targetProfile, integrated, lra, shortTerm, momentary, -60.0f, -60.0f);

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
            auto* alert = new juce::AlertWindow("Save Custom Layout", "Enter a name for the current dashboard layout:", juce::AlertWindow::QuestionIcon);
            alert->addTextEditor("layoutName", "Custom Layout", "Layout Name:");
            alert->addButton("Save",   1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
            alert->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));
            alert->enterModalState(true, juce::ModalCallbackFunction::create([this, alert](int result)
            {
                if (result == 1)
                {
                    juce::String name = alert->getTextEditorContents("layoutName").trim();
                    if (name.isNotEmpty())
                        saveCurrentLayout(name);
                }
                populateLayoutPresets();
            }));
        }
    };
}

void FF360MeterEditor::loadLayout (const ff360_labs::DashboardLayout& layout)
{
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

    auto existingActive = audioProcessor.apvts.state.getChildWithName("ActiveLayout");
    if (existingActive.isValid())
        audioProcessor.apvts.state.removeChild(existingActive, nullptr);
    
    auto newActive = layout.toValueTree("ActiveLayout");
    audioProcessor.apvts.state.addChild(newActive, -1, nullptr);
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

    inputDeviceComboBox.addItem("In: DAW Host Audio", 1);
    inputDeviceComboBox.setSelectedId(1, juce::dontSendNotification);
    inputDeviceComboBox.setEnabled(false);
}

void FF360MeterEditor::paint (juce::Graphics& g)
{
    g.fillAll(ff360_labs::BackgroundDark);

    // Header Bar
    auto headerRect = getLocalBounds().removeFromTop(40).toFloat();
    g.setColour(ff360_labs::ContainerDark);
    g.fillRect(headerRect);
    
    // Hairline bottom border
    g.setColour(ff360_labs::HairlineBorder);
    g.drawHorizontalLine((int)headerRect.getBottom() - 1, headerRect.getX(), headerRect.getRight());

    // Brand Title
    g.setFont(FF360LabsLookAndFeel::getCustomFont(15.0f, juce::Font::bold));
    g.setColour(ff360_labs::AccentGold);
    g.drawText("ff360_labs", headerRect.toNearestInt().withTrimmedLeft(20), juce::Justification::centredLeft, true);
    
    g.setColour(ff360_labs::TextMuted);
    g.drawText(" // ", headerRect.toNearestInt().withTrimmedLeft(110), juce::Justification::centredLeft, true);

    // Mockup keeps the brand subtitle dim like the separator, not bright white —
    // gold/brightness is reserved for the brand name and live data, not chrome.
    g.setColour(ff360_labs::TextMuted);
    g.drawText("MODULAR METER", headerRect.toNearestInt().withTrimmedLeft(135), juce::Justification::centredLeft, true);
}

void FF360MeterEditor::resized()
{
    auto bounds = getLocalBounds();
    auto headerRect = bounds.removeFromTop(40);
    
    // Right-to-left: status dots, settings button, layout combo, add module combo, input device combo, DEV OSC button
    ioStatusDot.setBounds(headerRect.removeFromRight(18).reduced(0, 8));
    perfDot.setBounds(headerRect.removeFromRight(18).reduced(0, 8));
    headerRect.removeFromRight(4); // gap
    btnSettings.setBounds(headerRect.removeFromRight(34).reduced(2, 6));
    headerRect.removeFromRight(4); // gap
    layoutComboBox.setBounds(headerRect.removeFromRight(135).reduced(2, 6));
    addModuleComboBox.setBounds(headerRect.removeFromRight(130).reduced(2, 6));
    inputDeviceComboBox.setBounds(headerRect.removeFromRight(175).reduced(2, 6));
    headerRect.removeFromRight(4); // gap
    btnDevOsc.setBounds(headerRect.removeFromRight(72).reduced(2, 6));
    
    meterDashboard.setBounds(bounds.reduced(8));
}
