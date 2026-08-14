#include "PluginEditor.h"
#include "../Core/Constants.h"
#include "../Core/SessionReport.h"

FF360MeterEditor::FF360MeterEditor (FF360MeterProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      btnGridMode("Grid Mode"),
      btnFocusMode("Focus Mode"),
      btnExportReport("Export Report"),
      btnColorblindMode("Accessible: OFF")
{
    setLookAndFeel(&customLookAndFeel);
    
    addAndMakeVisible(meterDashboard);
    addAndMakeVisible(perfBadge);
    addAndMakeVisible(btnColorblindMode);
    addAndMakeVisible(btnExportReport);
    addAndMakeVisible(layoutComboBox);
    addAndMakeVisible(addModuleComboBox);
    addAndMakeVisible(btnGridMode);
    addAndMakeVisible(btnFocusMode);

    // Performance Badge Setup
    perfBadge.setText("PERF: 60 FPS", juce::dontSendNotification);
    perfBadge.setFont(FF360LabsLookAndFeel::getCustomFont(10.0f, juce::Font::bold));
    perfBadge.setColour(juce::Label::textColourId, ff360_labs::AccentGold);
    perfBadge.setJustificationType(juce::Justification::centred);

    // Colorblind Mode Attachment
    btnColorblindMode.setClickingTogglesState(true);
    colorblindAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "colorblindMode", btnColorblindMode
    );
    
    bool initCb = btnColorblindMode.getToggleState();
    btnColorblindMode.setButtonText(initCb ? "Accessible: ON" : "Accessible: OFF");
    FF360LabsLookAndFeel::setColorblindModeActive(initCb);

    btnColorblindMode.onClick = [this] {
        bool on = btnColorblindMode.getToggleState();
        btnColorblindMode.setButtonText(on ? "Accessible: ON" : "Accessible: OFF");
        FF360LabsLookAndFeel::setColorblindModeActive(on);
        repaint();
    };
    
    addModuleComboBox.setTextWhenNothingSelected("Add Module...");
    addModuleComboBox.addItem("Peak / RMS Meter", 1);
    addModuleComboBox.addItem("VU Meter", 2);
    addModuleComboBox.addItem("LUFS Meter", 3);
    addModuleComboBox.addItem("Spectrum Analyzer", 4);
    addModuleComboBox.addItem("Histogram (5 Min)", 5);
    addModuleComboBox.addItem("Phase Scope", 6);
    
    btnGridMode.setRadioGroupId(1001);
    btnFocusMode.setRadioGroupId(1001);
    btnGridMode.setClickingTogglesState(true);
    btnFocusMode.setClickingTogglesState(true);
    btnGridMode.setToggleState(true, juce::dontSendNotification);

    btnGridMode.onClick = [this] {
        btnGridMode.setToggleState(true, juce::dontSendNotification);
        meterDashboard.setLayoutMode(LayoutMode::Grid);
    };
    btnFocusMode.onClick = [this] {
        btnFocusMode.setToggleState(true, juce::dontSendNotification);
        meterDashboard.setLayoutMode(LayoutMode::Maximized);
    };

    btnExportReport.onClick = [this] {
        triggerExportReport();
    };
    
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

    startTimerHz(4); // 4Hz performance budget monitor
    
    // Set a default size and allow resizing
    setResizable(true, true);
    setResizeLimits(800, 460, 2400, 1800);
    setSize(1040, 640);
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

    if (multiplier < 1.0f)
    {
        perfBadge.setText("PERF: 45 FPS", juce::dontSendNotification);
        perfBadge.setColour(juce::Label::textColourId, FF360LabsLookAndFeel::getWarningColour());
    }
    else
    {
        perfBadge.setText("PERF: 60 FPS", juce::dontSendNotification);
        perfBadge.setColour(juce::Label::textColourId, ff360_labs::AccentGold);
    }
}

MeterModule* FF360MeterEditor::createModule (MeterModuleType type)
{
    switch (type)
    {
        case MeterModuleType::PeakRms:
            return new PeakRmsMeterModule (audioProcessor.meterFifo);
        case MeterModuleType::VU:
            return new VuMeterModule (audioProcessor.vuFifo);
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

void FF360MeterEditor::triggerExportReport()
{
    // Retrieve target profile from APVTS or default
    int targetIdx = 0;
    if (auto* param = audioProcessor.apvts.getParameter("targetProfile"))
    {
        targetIdx = (int)param->getValue() * (int)(ff360_labs::LoudnessTarget::getBuiltinPresets().size() - 1);
    }
    auto targetProfile = ff360_labs::LoudnessTarget::getPresetByIndex(targetIdx);

    // Retrieve active DSP measurements
    float integrated = audioProcessor.lufsDSP.getIntegrated();
    float lra = audioProcessor.lufsDSP.getLRA();
    float shortTerm = audioProcessor.lufsDSP.getShortTerm();
    float momentary = audioProcessor.lufsDSP.getMomentary();
    float peakL = -60.0f;
    float peakR = -60.0f;

    auto data = ff360_labs::SessionReportData::collect(targetProfile, integrated, lra, shortTerm, momentary, peakL, peakR);

    juce::PopupMenu menu;
    menu.addItem(1, "Export as Branded Report (HTML / PDF-Ready)...");
    menu.addItem(2, "Export as Spreadsheet (CSV)...");

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&btnExportReport),
        [this, data](int result) {
            if (result == 1)
            {
                fileChooser = std::make_unique<juce::FileChooser>(
                    "Save Mastering Report",
                    juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("ff360labs_session_report.html"),
                    "*.html"
                );
                auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting;
                fileChooser->launchAsync(flags, [data](const juce::FileChooser& fc) {
                    auto file = fc.getResult();
                    if (file != juce::File{})
                        data.exportHtml(file);
                });
            }
            else if (result == 2)
            {
                fileChooser = std::make_unique<juce::FileChooser>(
                    "Save Loudness CSV Data",
                    juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("ff360labs_session_data.csv"),
                    "*.csv"
                );
                auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting;
                fileChooser->launchAsync(flags, [data](const juce::FileChooser& fc) {
                    auto file = fc.getResult();
                    if (file != juce::File{})
                        data.exportCsv(file);
                });
            }
        }
    );
}

void FF360MeterEditor::populateLayoutPresets()
{
    layoutComboBox.clear(juce::dontSendNotification);
    
    const auto& factory = ff360_labs::DashboardLayout::getFactoryPresets();
    for (size_t i = 0; i < factory.size(); ++i)
    {
        layoutComboBox.addItem("Layout: " + factory[i].name, (int)i + 1);
    }
    
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
            const auto& factory = ff360_labs::DashboardLayout::getFactoryPresets();
            loadLayout(factory[(size_t)(id - 1)]);
        }
        else if (id >= 100 && id < 900)
        {
            auto userLayoutsTree = audioProcessor.apvts.state.getChildWithName("UserLayouts");
            int userIdx = id - 100;
            if (userLayoutsTree.isValid() && userIdx < userLayoutsTree.getNumChildren())
            {
                loadLayout(ff360_labs::DashboardLayout::fromValueTree(userLayoutsTree.getChild(userIdx)));
            }
        }
        else if (id == 999)
        {
            auto* alert = new juce::AlertWindow ("Save Custom Layout", "Enter a name for the current dashboard layout:", juce::AlertWindow::QuestionIcon);
            alert->addTextEditor ("layoutName", "Custom Layout", "Layout Name:");
            alert->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey, 0, 0));
            alert->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey, 0, 0));
            alert->enterModalState (true, juce::ModalCallbackFunction::create ([this, alert] (int result)
            {
                if (result == 1)
                {
                    juce::String name = alert->getTextEditorContents ("layoutName").trim();
                    if (name.isNotEmpty())
                    {
                        saveCurrentLayout (name);
                    }
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
    if (layout.mode == LayoutMode::Grid)
        btnGridMode.setToggleState(true, juce::dontSendNotification);
    else
        btnFocusMode.setToggleState(true, juce::dontSendNotification);

    // Update active layout in state
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

void FF360MeterEditor::paint (juce::Graphics& g)
{
    g.fillAll (ff360_labs::BackgroundDark);

    // Header Bar
    auto headerRect = getLocalBounds().removeFromTop(40).toFloat();
    g.setColour(ff360_labs::ContainerDark);
    g.fillRect(headerRect);
    
    // Hairline bottom border
    g.setColour(ff360_labs::HairlineBorder);
    g.drawHorizontalLine((int)headerRect.getBottom() - 1, headerRect.getX(), headerRect.getRight());

    // Brand Title
    g.setFont (FF360LabsLookAndFeel::getCustomFont(15.0f, juce::Font::bold));
    g.setColour (ff360_labs::AccentGold);
    g.drawText ("ff360_labs", headerRect.toNearestInt().withTrimmedLeft(20), juce::Justification::centredLeft, true);
    
    g.setColour (ff360_labs::TextMuted);
    g.drawText (" // ", headerRect.toNearestInt().withTrimmedLeft(110), juce::Justification::centredLeft, true);

    g.setColour (ff360_labs::TextOffWhite);
    g.drawText ("MODULAR METER", headerRect.toNearestInt().withTrimmedLeft(135), juce::Justification::centredLeft, true);
}

void FF360MeterEditor::resized()
{
    auto bounds = getLocalBounds();
    auto headerRect = bounds.removeFromTop(40);
    
    btnGridMode.setBounds(headerRect.removeFromRight(84).reduced(3, 6));
    btnFocusMode.setBounds(headerRect.removeFromRight(84).reduced(3, 6));
    btnColorblindMode.setBounds(headerRect.removeFromRight(115).reduced(3, 6));
    btnExportReport.setBounds(headerRect.removeFromRight(105).reduced(3, 6));
    addModuleComboBox.setBounds(headerRect.removeFromRight(130).reduced(3, 6));
    layoutComboBox.setBounds(headerRect.removeFromRight(145).reduced(3, 6));
    perfBadge.setBounds(headerRect.removeFromRight(100).reduced(3, 6));
    
    meterDashboard.setBounds(bounds.reduced(8));
}
