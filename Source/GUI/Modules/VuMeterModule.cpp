#include "VuMeterModule.h"
#include "../../Core/Constants.h"
#include "../LookAndFeel/FF360LabsLookAndFeel.h"

VuMeterModule::VuMeterModule(AudioFifo<VuMeterData>& fifoToUse, VuDSP* dsp, juce::AudioProcessorValueTreeState* apvts)
    : MeterModule("VU METER", MeterModuleType::VU), meterFifo(fifoToUse), vuDSPInstance(dsp)
{
    debugHistory.resize(DEBUG_HISTORY_SIZE);

    // Populate Calibration Reference Levels
    const auto& presets = VuDSP::getCalibrationPresets();
    for (size_t i = 0; i < presets.size(); ++i)
    {
        calibrationSelector.addItem(presets[i].name, (int)i + 1);
    }

    if (apvts != nullptr)
    {
        calibrationAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            *apvts, "vuRefLevel", calibrationSelector);
    }
    else
    {
        calibrationSelector.setSelectedId(1);
    }

    int initIdx = calibrationSelector.getSelectedItemIndex();
    if (initIdx >= 0 && initIdx < (int)presets.size())
        currentRefLevelDb = presets[(size_t)initIdx].refDb;

    calibrationSelector.onChange = [this] {
        int idx = calibrationSelector.getSelectedItemIndex();
        const auto& p = VuDSP::getCalibrationPresets();
        if (idx >= 0 && idx < (int)p.size())
        {
            currentRefLevelDb = p[(size_t)idx].refDb;
            if (vuDSPInstance != nullptr)
                vuDSPInstance->setReferenceLevelDb(currentRefLevelDb);
        }
        repaint();
    };

    addAndMakeVisible(calibrationSelector);

    // Dev Debug Overlay Button
    debugButton.setClickingTogglesState(true);
    debugButton.setColour(juce::TextButton::buttonOnColourId, ff360_labs::AccentAmberRed.withAlpha(0.6f));
    debugButton.onClick = [this] {
        showDebugOverlay = debugButton.getToggleState();
        repaint();
    };
    addAndMakeVisible(debugButton);

    startTimerHz(60);
}

VuMeterModule::~VuMeterModule()
{
    stopTimer();
}

void VuMeterModule::timerCallback()
{
    VuMeterData newData;
    if (meterFifo.pullLatest(newData))
    {
        currentData = newData;
    }

    // Phase 9.1: Single Source of Timing Truth
    // Direct 1:1 needle tracking from DSP ballistics (no stacked spring lag)
    renderedVuL = currentData.vuL;
    renderedVuR = currentData.vuR;

    // Record sample in debug history ring buffer
    DebugSample s;
    s.dspVu = currentData.vuL;
    s.needleVu = renderedVuL;
    s.rawDbfs = currentData.rawDbfslL;
    debugHistory[debugWriteIndex] = s;
    debugWriteIndex = (debugWriteIndex + 1) % DEBUG_HISTORY_SIZE;

    repaint();
}

void VuMeterModule::drawVuArcGauge (juce::Graphics& g, juce::Rectangle<float> bounds, float vuValue, const juce::String& channelLabel)
{
    // Draw Glass panel container for the channel gauge
    FF360LabsLookAndFeel::drawGlassPanel(g, bounds, 6.0f);

    auto inner = bounds.reduced(6.0f);

    // Channel label at the top
    auto labelArea = inner.removeFromTop(15.0f);
    g.setFont(FF360LabsLookAndFeel::getCustomFont(10.0f, juce::Font::bold));
    g.setColour(ff360_labs::TextMuted);
    g.drawText(channelLabel, labelArea, juce::Justification::centred, false);

    // Digital Readout Stat-Tile at the bottom
    auto readoutArea = inner.removeFromBottom(26.0f).reduced(inner.getWidth() * 0.12f, 0.0f);
    inner.removeFromBottom(4.0f); // Gap above readout

    // Responsive Arc Geometry Fitting
    float w = inner.getWidth();
    float h = inner.getHeight();

    float minVu = -20.0f;
    float maxVu = 3.0f;
    
    // Angles: -38 deg to +38 deg
    float minAngle = -juce::MathConstants<float>::pi * 0.21f;
    float maxAngle =  juce::MathConstants<float>::pi * 0.21f;
    float sinMax = std::sin(maxAngle);

    // Auto-fit radius to ensure neither width nor height ever clips
    float maxRadiusW = (w - 32.0f) / (2.0f * sinMax);
    float maxRadiusH = (h - 14.0f);
    float radius = juce::jmax(25.0f, std::min(maxRadiusW, maxRadiusH));
    float arcThickness = juce::jlimit(6.0f, 10.0f, radius * 0.10f);

    float cx = inner.getCentreX();
    float cy = inner.getY() + radius + 12.0f;
    juce::Point<float> pivot(cx, cy);

    // 1. Brushed charcoal arc track
    juce::Path arcTrack;
    arcTrack.addCentredArc(cx, cy, radius, radius, 0.0f, minAngle, maxAngle, true);

    g.setColour(ff360_labs::ContainerDark.darker(0.35f));
    g.strokePath(arcTrack, juce::PathStrokeType(arcThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 2. Warning zone arc (0 to +3 VU, upper ~10%)
    float zeroNorm = (0.0f - minVu) / (maxVu - minVu);
    float zeroAngle = minAngle + zeroNorm * (maxAngle - minAngle);

    juce::Path warningArc;
    warningArc.addCentredArc(cx, cy, radius, radius, 0.0f, zeroAngle, maxAngle, true);
    g.setColour(ff360_labs::AccentAmberRed.withAlpha(0.40f));
    g.strokePath(warningArc, juce::PathStrokeType(arcThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 3. Hairline guide along outer arc
    juce::Path hairlineTrack;
    hairlineTrack.addCentredArc(cx, cy, radius + arcThickness * 0.5f, radius + arcThickness * 0.5f, 0.0f, minAngle, maxAngle, true);
    g.setColour(ff360_labs::HairlineBorder);
    g.strokePath(hairlineTrack, juce::PathStrokeType(1.0f));

    // 4. Calibration Ticks and Numbers
    g.setFont(FF360LabsLookAndFeel::getCustomFont(8.5f, juce::Font::plain));
    
    const int tickValues[] = { -20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3 };
    for (int v : tickValues)
    {
        float norm = (v - minVu) / (maxVu - minVu);
        float angle = minAngle + norm * (maxAngle - minAngle);

        bool isOverZero = (v > 0);
        bool isMajor = (v == -20 || v == -10 || v == -5 || v == 0 || v == 3);
        float tickLen = isMajor ? (arcThickness * 0.6f) : (arcThickness * 0.35f);

        juce::Point<float> pOuter = pivot.getPointOnCircumference(radius + arcThickness * 0.5f, angle);
        juce::Point<float> pInner = pivot.getPointOnCircumference(radius + arcThickness * 0.5f - tickLen, angle);

        g.setColour(isOverZero ? ff360_labs::AccentAmberRed : ff360_labs::AccentGold.withAlpha(isMajor ? 0.9f : 0.45f));
        g.drawLine(juce::Line<float>(pInner, pOuter), isMajor ? 1.4f : 0.9f);

        if (isMajor)
        {
            juce::Point<float> textPos = pivot.getPointOnCircumference(radius + arcThickness * 0.5f + 7.0f, angle);
            juce::String str = (v > 0 ? "+" : "") + juce::String(v);
            g.setColour(isOverZero ? ff360_labs::AccentAmberRed : ff360_labs::TextOffWhite);
            g.drawText(str, juce::Rectangle<float>(textPos.x - 12.0f, textPos.y - 5.0f, 24.0f, 10.0f),
                       juce::Justification::centred, false);
        }
    }

    // 0 VU Reference Sub-Label on Dial Face
    g.setFont(FF360LabsLookAndFeel::getCustomFont(7.5f, juce::Font::plain));
    g.setColour(ff360_labs::TextMuted);
    juce::String calRefLabel = "0 VU = " + juce::String((int)currentRefLevelDb) + " dBFS";
    g.drawText(calRefLabel, juce::Rectangle<float>(cx - 45.0f, cy - radius * 0.45f, 90.0f, 12.0f), juce::Justification::centred, false);

    // 5. Direct 1:1 Rendered Needle (Instantaneous, exact ballistics tracking)
    float clampedVu = juce::jlimit(minVu, maxVu, vuValue);
    float normalizedAngle = (clampedVu - minVu) / (maxVu - minVu);
    float needleAngle = minAngle + normalizedAngle * (maxAngle - minAngle);

    juce::Point<float> needleEnd = pivot.getPointOnCircumference(radius + arcThickness * 0.3f, needleAngle);

    // Soft Needle Glow Trail
    g.setColour(ff360_labs::GoldGlow);
    g.drawLine(juce::Line<float>(pivot, needleEnd), 3.0f);

    // Sharp Metallic Gold Needle
    g.setColour(clampedVu > 0.0f ? ff360_labs::AccentAmberRed : ff360_labs::AccentGold);
    g.drawLine(juce::Line<float>(pivot, needleEnd), 1.5f);

    // Pivot Shadow & Cover
    float pivotRadius = juce::jlimit(4.0f, 6.0f, radius * 0.06f);
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillEllipse(pivot.x - pivotRadius - 1.0f, pivot.y - pivotRadius, (pivotRadius + 1.0f) * 2.0f, (pivotRadius + 1.0f) * 2.0f);

    g.setColour(ff360_labs::ContainerDark);
    g.fillEllipse(pivot.x - pivotRadius, pivot.y - pivotRadius, pivotRadius * 2.0f, pivotRadius * 2.0f);

    g.setColour(ff360_labs::AccentGold);
    g.drawEllipse(pivot.x - pivotRadius, pivot.y - pivotRadius, pivotRadius * 2.0f, pivotRadius * 2.0f, 1.2f);
    g.fillEllipse(pivot.x - 2.0f, pivot.y - 2.0f, 4.0f, 4.0f);

    // 6. Digital Readout Stat-Tile
    juce::String valStr = (clampedVu > 0.0f ? "+" : "") + juce::String(clampedVu, 1) + " VU";
    bool isWarning = (clampedVu >= 0.0f);
    FF360LabsLookAndFeel::drawStatTile(g, readoutArea, "READOUT", valStr, isWarning);
}

void VuMeterModule::drawDebugOverlay (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Semi-transparent diagnostic HUD
    auto hudArea = bounds.removeFromBottom(110.0f).reduced(8.0f);
    g.setColour(juce::Colours::black.withAlpha(0.85f));
    g.fillRoundedRectangle(hudArea, 6.0f);
    g.setColour(ff360_labs::AccentAmberRed.withAlpha(0.8f));
    g.drawRoundedRectangle(hudArea, 6.0f, 1.2f);

    auto header = hudArea.removeFromTop(16.0f).reduced(4.0f, 0.0f);
    g.setFont(FF360LabsLookAndFeel::getCustomFont(9.0f, juce::Font::bold));
    g.setColour(ff360_labs::AccentAmberRed);
    g.drawText("DEV TIMING OSCILLOSCOPE // DSP BALLISTICS vs RENDERED NEEDLE", header, juce::Justification::centredLeft, false);

    // Scope Graph area
    auto plotRect = hudArea.reduced(6.0f);
    g.setColour(ff360_labs::ContainerDark.darker(0.6f));
    g.fillRect(plotRect);
    g.setColour(ff360_labs::HairlineBorder);
    g.drawRect(plotRect, 1.0f);

    // Grid lines: 0 VU line and -10 VU line
    float yZero = plotRect.getY() + plotRect.getHeight() * (3.0f - 0.0f) / 23.0f;
    g.setColour(ff360_labs::AccentAmberRed.withAlpha(0.3f));
    g.drawHorizontalLine((int)yZero, plotRect.getX(), plotRect.getRight());

    // Render DSP VU trace (Gold) vs Needle Angle (Cyan)
    juce::Path dspPath;
    juce::Path needlePath;

    size_t count = debugHistory.size();
    for (size_t i = 0; i < count; ++i)
    {
        size_t ringIdx = (debugWriteIndex + i) % count;
        const auto& sample = debugHistory[ringIdx];

        float x = plotRect.getX() + (float)i / (float)(count - 1) * plotRect.getWidth();
        
        // Scale VU (-20 to +3) to Y
        float normDsp = juce::jlimit(0.0f, 1.0f, (sample.dspVu - (-20.0f)) / 23.0f);
        float normNeedle = juce::jlimit(0.0f, 1.0f, (sample.needleVu - (-20.0f)) / 23.0f);

        float yDsp = plotRect.getBottom() - normDsp * plotRect.getHeight();
        float yNeedle = plotRect.getBottom() - normNeedle * plotRect.getHeight();

        if (i == 0)
        {
            dspPath.startNewSubPath(x, yDsp);
            needlePath.startNewSubPath(x, yNeedle);
        }
        else
        {
            dspPath.lineTo(x, yDsp);
            needlePath.lineTo(x, yNeedle);
        }
    }

    g.setColour(ff360_labs::AccentGold.withAlpha(0.9f));
    g.strokePath(dspPath, juce::PathStrokeType(1.5f));

    g.setColour(juce::Colour(0xff00e5ff).withAlpha(0.7f));
    g.strokePath(needlePath, juce::PathStrokeType(1.0f));

    // Legend
    g.setFont(FF360LabsLookAndFeel::getCustomFont(8.0f, juce::Font::plain));
    g.setColour(ff360_labs::AccentGold);
    g.drawText("Gold: Raw DSP (120ms True RMS)", juce::Rectangle<float>(plotRect.getX() + 4.0f, plotRect.getY() + 2.0f, 160.0f, 10.0f), juce::Justification::left, false);
    g.setColour(juce::Colour(0xff00e5ff));
    g.drawText("Cyan: Needle Angle (Direct 1:1, 0ms Lag)", juce::Rectangle<float>(plotRect.getX() + 170.0f, plotRect.getY() + 2.0f, 190.0f, 10.0f), juce::Justification::left, false);
}

void VuMeterModule::paintModule(juce::Graphics& g)
{
    auto fullBounds = getModuleBounds().toFloat();
    
    // Top Control Strip Area (Module Header)
    auto topStrip = fullBounds.removeFromTop(24.0f).reduced(4.0f, 2.0f);

    auto gaugeBounds = fullBounds.reduced(4.0f);
    float laneWidth = (gaugeBounds.getWidth() - 8.0f) * 0.5f;
    
    juce::Rectangle<float> leftLane(gaugeBounds.getX(), gaugeBounds.getY(), laneWidth, gaugeBounds.getHeight());
    juce::Rectangle<float> rightLane(gaugeBounds.getRight() - laneWidth, gaugeBounds.getY(), laneWidth, gaugeBounds.getHeight());
    
    drawVuArcGauge(g, leftLane, renderedVuL, "CH 1 // LEFT");
    drawVuArcGauge(g, rightLane, renderedVuR, "CH 2 // RIGHT");

    if (showDebugOverlay)
    {
        drawDebugOverlay(g, gaugeBounds);
    }
}

void VuMeterModule::resizedModule()
{
    auto bounds = getModuleBounds().reduced(4);
    auto topStrip = bounds.removeFromTop(22);

    calibrationSelector.setBounds(topStrip.removeFromLeft(210));
    topStrip.removeFromLeft(6);
    debugButton.setBounds(topStrip.removeFromLeft(70));
}


