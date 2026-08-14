#include "VuMeterModule.h"
#include "../../Core/Constants.h"
#include "../LookAndFeel/FF360LabsLookAndFeel.h"

VuMeterModule::VuMeterModule(AudioFifo<VuMeterData>& fifoToUse)
    : MeterModule("VU METER", MeterModuleType::VU), meterFifo(fifoToUse)
{
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

    needleL.setTarget(currentData.vuL);
    needleR.setTarget(currentData.vuR);

    needleL.update(1.0f / 60.0f);
    needleR.update(1.0f / 60.0f);

    repaint();
}

void VuMeterModule::drawVuArcGauge (juce::Graphics& g, juce::Rectangle<float> bounds, float smoothedVu, const juce::String& channelLabel)
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

    // 5. Spring-Damped Needle
    float clampedVu = juce::jlimit(minVu, maxVu, smoothedVu);
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

void VuMeterModule::paintModule(juce::Graphics& g)
{
    auto bounds = getModuleBounds().toFloat().reduced(6.0f);
    
    float laneWidth = (bounds.getWidth() - 8.0f) * 0.5f;
    
    juce::Rectangle<float> leftLane(bounds.getX(), bounds.getY(), laneWidth, bounds.getHeight());
    juce::Rectangle<float> rightLane(bounds.getRight() - laneWidth, bounds.getY(), laneWidth, bounds.getHeight());
    
    drawVuArcGauge(g, leftLane, needleL.getCurrent(), "CH 1 // LEFT");
    drawVuArcGauge(g, rightLane, needleR.getCurrent(), "CH 2 // RIGHT");
}

void VuMeterModule::resizedModule()
{
}

