#include "AudioSettingsModal.h"
#include "../LookAndFeel/FF360LabsLookAndFeel.h"
#include "../../Core/Constants.h"

AudioSettingsModal::AudioSettingsModal(FF360MeterProcessor& processor, juce::AudioDeviceManager* devMgr)
    : audioProcessor(processor), deviceManager(devMgr)
{
    if (deviceManager != nullptr)
    {
        // 0-2 input channels, 0-2 output channels, show sample rates, buffer sizes
        deviceSelector = std::make_unique<juce::AudioDeviceSelectorComponent>(
            *deviceManager,
            1, 2,  // min/max audio input channels
            0, 2,  // min/max audio output channels
            false, // show MIDI input options
            false, // show MIDI output options
            true,  // show channels as stereo pairs
            false  // hide advanced options with button
        );
        deviceSelector->setLookAndFeel(&FF360LabsLookAndFeel::getDefaultLookAndFeel());
        addAndMakeVisible(deviceSelector.get());
    }

    addAndMakeVisible(btnClose);
    btnClose.onClick = [this] {
        if (onClose)
            onClose();
        if (auto* parent = getParentComponent())
            parent->removeChildComponent(this);
    };

    inspectPlatformLoopback();
    startTimerHz(15);
    setSize(640, 520);
}

AudioSettingsModal::~AudioSettingsModal()
{
    stopTimer();
    if (deviceSelector != nullptr)
        deviceSelector->setLookAndFeel(nullptr);
}

void AudioSettingsModal::inspectPlatformLoopback()
{
    hasLoopbackDevice = false;
    detectedLoopbackName = "";

    if (deviceManager != nullptr)
    {
        auto* currentType = deviceManager->getCurrentDeviceTypeObject();
        if (currentType != nullptr)
        {
            auto inputDevices = currentType->getDeviceNames(true);
            for (const auto& dev : inputDevices)
            {
                juce::String lower = dev.toLowerCase();
                if (lower.contains("blackhole") || lower.contains("loopback") || lower.contains("soundflower")
                    || lower.contains("stereo mix") || lower.contains("cable") || lower.contains("virtual")
                    || lower.contains("what u hear") || lower.contains("monitor"))
                {
                    hasLoopbackDevice = true;
                    detectedLoopbackName = dev;
                    break;
                }
            }
        }
    }

   #if JUCE_WINDOWS
    osGuidanceText = "WINDOWS WASAPI / SYSTEM AUDIO (Phase 9.3 \xe2\x80\x94 confirmed):\n"
                     "Select 'Windows Audio (WASAPI)' or 'DirectSound' as the device type. "
                     "To monitor desktop audio (WASAPI Loopback), choose 'Stereo Mix' "
                     "or a virtual loopback endpoint (e.g. VB-Cable / Virtual Audio Cable) as the Input Device. "
                     "Access this screen via the \xe2\x9a\x99 Settings menu in the top nav bar.";
   #elif JUCE_MAC
    osGuidanceText = "macOS SYSTEM AUDIO CAPTURE:\n"
                     "macOS requires an audio routing driver to capture desktop output. Install BlackHole (2ch) or Rogue Amoeba Loopback, "
                     "set system output to Multi-Output Device, and select BlackHole as the Input Device above.";
   #elif JUCE_LINUX
    osGuidanceText = "LINUX SYSTEM AUDIO CAPTURE:\n"
                     "Select the PipeWire / PulseAudio 'Monitor of [Output Device]' as the Input Source to route system audio.";
   #else
    osGuidanceText = "Select your desired system audio or hardware input device from the list above.";
   #endif
}

void AudioSettingsModal::timerCallback()
{
    repaint();
}

void AudioSettingsModal::renderStatusBadge(juce::Graphics& g, juce::Rectangle<float> area)
{
    bool isConnected = audioProcessor.getIsInputConnected();
    bool isSilent = audioProcessor.getIsAudioSilent();
    float peakDb = audioProcessor.getCurrentPeakLevelDb();

    juce::Colour badgeCol;
    juce::String badgeText;

    if (!isConnected)
    {
        badgeCol = ff360_labs::AccentAmberRed;
        badgeText = "● NO INPUT DEVICE CONNECTED";
    }
    else if (isSilent)
    {
        badgeCol = ff360_labs::AccentGold.withAlpha(0.7f);
        badgeText = "● INPUT ACTIVE // SILENT (IDLE)";
    }
    else
    {
        badgeCol = juce::Colour(0xff00e5ff);
        badgeText = "● LIVE CAPTURE STREAMING [" + juce::String(peakDb, 1) + " dBFS]";
    }

    g.setColour(badgeCol.withAlpha(0.18f));
    g.fillRoundedRectangle(area, 4.0f);
    g.setColour(badgeCol);
    g.drawRoundedRectangle(area, 4.0f, 1.0f);

    g.setFont(FF360LabsLookAndFeel::getCustomFont(10.0f, juce::Font::bold));
    g.setColour(badgeCol);
    g.drawText(badgeText, area.reduced(8.0f, 0.0f), juce::Justification::centredLeft, true);
}

void AudioSettingsModal::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black.withAlpha(0.65f));

    auto cardRect = getLocalBounds().toFloat().reduced(20.0f);
    FF360LabsLookAndFeel::drawGlassPanel(g, cardRect, 8.0f);

    // Header
    auto headerRect = cardRect.removeFromTop(44.0f).reduced(12.0f, 6.0f);
    g.setFont(FF360LabsLookAndFeel::getCustomFont(14.0f, juce::Font::bold));
    g.setColour(ff360_labs::AccentGold);
    g.drawText("STANDALONE AUDIO I/O & SYSTEM LOOPBACK ROUTING", headerRect, juce::Justification::centredLeft, true);

    // Live status badge
    auto statusArea = cardRect.removeFromTop(30.0f).reduced(12.0f, 2.0f);
    renderStatusBadge(g, statusArea);

    // Bottom Guidance Card
    auto bottomArea = cardRect.removeFromBottom(100.0f).reduced(12.0f, 4.0f);
    g.setColour(ff360_labs::ContainerDark.darker(0.4f));
    g.fillRoundedRectangle(bottomArea, 6.0f);
    g.setColour(ff360_labs::HairlineBorder);
    g.drawRoundedRectangle(bottomArea, 6.0f, 1.0f);

    auto textRect = bottomArea.reduced(8.0f);
    g.setFont(FF360LabsLookAndFeel::getCustomFont(9.0f, juce::Font::plain));
    g.setColour(ff360_labs::TextOffWhite);
    g.drawFittedText(osGuidanceText, textRect.toNearestInt(), juce::Justification::topLeft, 4);

    if (hasLoopbackDevice)
    {
        auto detectedTag = bottomArea.removeFromBottom(16.0f).reduced(8.0f, 0.0f);
        g.setFont(FF360LabsLookAndFeel::getCustomFont(9.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xff00e5ff));
        g.drawText("✔ Loopback Driver Detected: " + detectedLoopbackName, detectedTag, juce::Justification::bottomLeft, false);
    }
}

void AudioSettingsModal::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    bounds.removeFromTop(80); // Title and status badge
    auto bottom = bounds.removeFromBottom(140);
    
    btnClose.setBounds(bottom.removeFromBottom(32).removeFromRight(100));

    if (deviceSelector != nullptr)
        deviceSelector->setBounds(bounds.reduced(10));
}

void AudioSettingsModal::showModal(juce::Component* parent, FF360MeterProcessor& processor, juce::AudioDeviceManager* deviceManager, std::function<void()> onClosed)
{
    if (parent == nullptr) return;

    auto* modal = new AudioSettingsModal(processor, deviceManager);
    modal->onClose = std::move(onClosed);
    modal->setBounds(parent->getLocalBounds());
    parent->addAndMakeVisible(modal);
}
