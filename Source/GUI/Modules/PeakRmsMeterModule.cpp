#include "PeakRmsMeterModule.h"
#include "../../Core/Constants.h"
#include "../LookAndFeel/FF360LabsLookAndFeel.h"

PeakRmsMeterModule::PeakRmsMeterModule(AudioFifo<MeterData>& fifoToUse)
    : MeterModule("PEAK / RMS METER", MeterModuleType::PeakRms), meterFifo(fifoToUse)
{
    // Configure ballistics for UI threading (assuming ~60fps)
    dsp.prepare(60.0, 1);
    startTimerHz(60);
}

PeakRmsMeterModule::~PeakRmsMeterModule()
{
    stopTimer();
}

void PeakRmsMeterModule::timerCallback()
{
    MeterData newData;
    if (meterFifo.pullLatest(newData))
    {
        dsp.applyBallistics(currentSmoothedData, newData);
        repaint();
    }
}

float PeakRmsMeterModule::jmapDbToHeight(float db, float height)
{
    // Maps [METER_FLOOR_DB, METER_CEILING_DB] to [height, 0]
    float range = ff360_labs::METER_CEILING_DB - ff360_labs::METER_FLOOR_DB;
    float normalized = (db - ff360_labs::METER_FLOOR_DB) / range;
    
    return height - (juce::jlimit(0.0f, 1.0f, normalized) * height);
}

void PeakRmsMeterModule::drawMeterLane(juce::Graphics& g, juce::Rectangle<float> bounds, float peakDb, float rmsDb)
{
    float cornerSize = 4.0f;

    // 1. Draw Background Track (Dark Matte Charcoal with subtle Hairline)
    g.setColour(ff360_labs::ContainerDark.darker(0.5f));
    g.fillRoundedRectangle(bounds, cornerSize);
    g.setColour(ff360_labs::HairlineBorder);
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 1.0f);

    auto inner = bounds.reduced(3.0f, 4.0f);
    
    // 2. Segment resolution: ~48 discrete horizontal LED segments
    int numSegments = juce::jlimit(24, 60, static_cast<int>(inner.getHeight() / 4.5f));
    float segPitch = inner.getHeight() / (float)numSegments;
    float segH = juce::jmax(2.0f, segPitch - 1.2f);
    float segW = inner.getWidth();

    int highestLitPeakSegment = -1;

    for (int s = 0; s < numSegments; ++s)
    {
        float segDb = ff360_labs::METER_FLOOR_DB + ((float)s / (float)(numSegments - 1)) * (ff360_labs::METER_CEILING_DB - ff360_labs::METER_FLOOR_DB);
        if (peakDb >= segDb)
            highestLitPeakSegment = s;
    }

    for (int s = 0; s < numSegments; ++s)
    {
        float segDb = ff360_labs::METER_FLOOR_DB + ((float)s / (float)(numSegments - 1)) * (ff360_labs::METER_CEILING_DB - ff360_labs::METER_FLOOR_DB);
        float segY = inner.getBottom() - ((float)(s + 1) * segPitch);
        juce::Rectangle<float> segRect(inner.getX(), segY, segW, segH);

        bool isPeakLit = (peakDb >= segDb);
        bool isRmsLit = (rmsDb >= segDb);
        bool isWarning = (segDb >= ff360_labs::METER_WARNING_DB);

        juce::Colour activeColor = isWarning ? FF360LabsLookAndFeel::getWarningColour() : ff360_labs::AccentGold;

        if (isPeakLit)
        {
            // If topmost lit segment, add enhanced glow accent
            if (s == highestLitPeakSegment)
            {
                g.setColour(activeColor.withAlpha(0.45f));
                g.drawRoundedRectangle(segRect.expanded(1.5f, 1.0f), 2.0f, 1.2f);
            }

            // Fill lit LED segment
            g.setColour(activeColor);
            g.fillRoundedRectangle(segRect, 1.5f);

            // RMS Core Luminance / inner bright notch
            if (isRmsLit)
            {
                g.setColour(ff360_labs::TextOffWhite.withAlpha(0.65f));
                g.fillRoundedRectangle(segRect.reduced(segW * 0.25f, 0.0f), 1.0f);
            }
        }
        else
        {
            // Unlit segment: Faintly visible charcoal-gold so scale remains clear
            g.setColour(isWarning ? ff360_labs::AccentAmberRed.withAlpha(0.08f)
                                  : ff360_labs::AccentGold.withAlpha(0.06f));
            g.fillRoundedRectangle(segRect, 1.5f);
        }
    }

    // 3. Distinct Horizontal -3dB Threshold Marker Line
    float thresholdY = jmapDbToHeight(ff360_labs::METER_WARNING_DB, bounds.getHeight());
    float markerY = bounds.getY() + thresholdY;
    g.setColour(FF360LabsLookAndFeel::getWarningColour());
    g.drawHorizontalLine((int)markerY, bounds.getX() - 1.0f, bounds.getRight() + 1.0f);
}

void PeakRmsMeterModule::paintModule(juce::Graphics& g)
{
    auto bounds = getModuleBounds().toFloat().reduced(8.0f);
    
    // Bottom area reserved for numeric stat tiles
    auto statArea = bounds.removeFromBottom(30.0f);
    bounds.removeFromBottom(8.0f); // Gap

    float totalW = bounds.getWidth();
    float laneW = (totalW - 40.0f) * 0.5f; // 40px center scale
    
    juce::Rectangle<float> leftLane(bounds.getX(), bounds.getY(), laneW, bounds.getHeight());
    juce::Rectangle<float> scaleArea(leftLane.getRight(), bounds.getY(), 40.0f, bounds.getHeight());
    juce::Rectangle<float> rightLane(scaleArea.getRight(), bounds.getY(), laneW, bounds.getHeight());
    
    drawMeterLane(g, leftLane, currentSmoothedData.peakL, currentSmoothedData.rmsL);
    drawMeterLane(g, rightLane, currentSmoothedData.peakR, currentSmoothedData.rmsR);
    
    // Draw Center dB Scale Ticks
    const float scaleDbs[] = { 0.0f, -3.0f, -6.0f, -12.0f, -18.0f, -24.0f, -36.0f, -48.0f, -60.0f };
    g.setFont(FF360LabsLookAndFeel::getCustomFont(9.0f, juce::Font::plain));

    for (float db : scaleDbs)
    {
        float y = bounds.getY() + jmapDbToHeight(db, bounds.getHeight());
        bool isWarning = (db >= ff360_labs::METER_WARNING_DB);

        g.setColour(isWarning ? ff360_labs::AccentAmberRed.withAlpha(0.6f) : ff360_labs::HairlineBorder);
        g.drawLine(scaleArea.getX() + 2.0f, y, scaleArea.getX() + 6.0f, y, 1.0f);
        g.drawLine(scaleArea.getRight() - 6.0f, y, scaleArea.getRight() - 2.0f, y, 1.0f);

        g.setColour(isWarning ? ff360_labs::AccentAmberRed : ff360_labs::TextMuted);
        juce::String str = (db == 0.0f ? "0" : juce::String((int)db));
        g.drawText(str, juce::Rectangle<float>(scaleArea.getX(), y - 6.0f, scaleArea.getWidth(), 12.0f),
                   juce::Justification::centred, false);
    }

    // Draw -3dB Horizontal Marker Line across Center Scale
    float thresholdY = bounds.getY() + jmapDbToHeight(ff360_labs::METER_WARNING_DB, bounds.getHeight());
    g.setColour(ff360_labs::AccentGold.withAlpha(0.35f));
    g.drawHorizontalLine((int)thresholdY, leftLane.getX(), rightLane.getRight());

    // Draw Numeric Stat-Tiles at Bottom
    float statTileW = (statArea.getWidth() - 8.0f) * 0.5f;
    juce::Rectangle<float> leftStat(statArea.getX(), statArea.getY(), statTileW, statArea.getHeight());
    juce::Rectangle<float> rightStat(statArea.getRight() - statTileW, statArea.getY(), statTileW, statArea.getHeight());

    juce::String leftPeakStr = juce::String(currentSmoothedData.peakL, 1) + " dB";
    juce::String rightPeakStr = juce::String(currentSmoothedData.peakR, 1) + " dB";

    bool leftWarn = (currentSmoothedData.peakL >= ff360_labs::METER_WARNING_DB);
    bool rightWarn = (currentSmoothedData.peakR >= ff360_labs::METER_WARNING_DB);

    FF360LabsLookAndFeel::drawStatTile(g, leftStat, "PEAK L", leftPeakStr, leftWarn);
    FF360LabsLookAndFeel::drawStatTile(g, rightStat, "PEAK R", rightPeakStr, rightWarn);
}

void PeakRmsMeterModule::resizedModule()
{
}

