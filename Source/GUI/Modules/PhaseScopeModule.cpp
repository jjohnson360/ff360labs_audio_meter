#include "PhaseScopeModule.h"
#include "../../Core/Constants.h"
#include "../LookAndFeel/FF360LabsLookAndFeel.h"

PhaseScopeModule::PhaseScopeModule(AudioFifo<PhaseScopeData>& fifoToUse)
    : MeterModule("PHASE SCOPE", MeterModuleType::PhaseScope), meterFifo(fifoToUse)
{
    scopeImage = juce::Image(juce::Image::ARGB, 100, 100, true);
    startTimerHz(60);
}

PhaseScopeModule::~PhaseScopeModule()
{
    stopTimer();
}

void PhaseScopeModule::timerCallback()
{
    PhaseScopeData newData;
    bool hasNewData = false;
    
    while (meterFifo.pull(newData))
    {
        currentData.correlation = newData.correlation;
        currentData.samplePairs.insert(currentData.samplePairs.end(), newData.samplePairs.begin(), newData.samplePairs.end());
        hasNewData = true;
    }
    
    if (hasNewData)
    {
        repaint();
    }
}

void PhaseScopeModule::updateScopeImage(juce::Rectangle<float> bounds)
{
    int w = juce::roundToInt(bounds.getWidth());
    int h = juce::roundToInt(bounds.getHeight());
    
    if (w <= 0 || h <= 0) return;
    
    if (scopeImage.getWidth() != w || scopeImage.getHeight() != h)
    {
        scopeImage = juce::Image(juce::Image::ARGB, w, h, true);
    }
    
    juce::Graphics g(scopeImage);
    
    // Smooth Fading persistence
    scopeImage.multiplyAllAlphas(0.82f);
    
    if (currentData.samplePairs.empty())
        return;
        
    float cx = w * 0.5f;
    float cy = h * 0.5f;
    float scale = std::min(cx, cy) * 0.88f; 
    
    size_t totalPoints = currentData.samplePairs.size();
    if (totalPoints < 2)
    {
        currentData.samplePairs.clear();
        return;
    }

    // Draw multi-segment path with age-dependent opacity
    size_t step = std::max<size_t>(1, totalPoints / 120);
    for (size_t i = step; i < totalPoints; i += step)
    {
        float ageNorm = (float)i / (float)totalPoints; // 0.0 (oldest) to 1.0 (newest)
        
        float l1 = currentData.samplePairs[i - step].x;
        float r1 = currentData.samplePairs[i - step].y;
        float l2 = currentData.samplePairs[i].x;
        float r2 = currentData.samplePairs[i].y;

        float mid1 = (l1 + r1) * 0.70710678f;
        float side1 = (r1 - l1) * 0.70710678f;
        float mid2 = (l2 + r2) * 0.70710678f;
        float side2 = (r2 - l2) * 0.70710678f;

        float p1x = cx + (side1 * scale);
        float p1y = cy - (mid1 * scale);
        float p2x = cx + (side2 * scale);
        float p2y = cy - (mid2 * scale);

        // Newer points shine brightly in metallic gold with soft glow
        float alpha = 0.15f + (ageNorm * 0.75f);
        g.setColour(ff360_labs::AccentGold.withAlpha(alpha));
        g.drawLine(p1x, p1y, p2x, p2y, 1.4f);
    }
    
    currentData.samplePairs.clear();
}

void PhaseScopeModule::drawCorrelationMeter(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    FF360LabsLookAndFeel::drawGlassPanel(g, bounds, 6.0f);
    auto inner = bounds.reduced(6.0f, 4.0f);

    float c = currentData.correlation;
    bool isWarning = (c < 0.0f);

    // Left title + Right Numeric Readout
    auto topArea = inner.removeFromTop(12.0f);
    g.setFont(FF360LabsLookAndFeel::getCustomFont(9.0f, juce::Font::bold));
    g.setColour(ff360_labs::TextMuted);
    g.drawText("CORRELATION", topArea, juce::Justification::centredLeft, false);

    juce::String valStr = (c >= 0.0f ? "+" : "") + juce::String(c, 2);
    g.setColour(FF360LabsLookAndFeel::getReadoutColour(isWarning));
    g.setFont(FF360LabsLookAndFeel::getNumericReadoutFont(11.0f));
    g.drawText(valStr, topArea, juce::Justification::centredRight, false);

    // Correlation Bar Track
    auto barBounds = inner.removeFromTop(10.0f).reduced(2.0f, 0.0f);
    g.setColour(ff360_labs::ContainerDark.darker(0.5f));
    g.fillRoundedRectangle(barBounds, 2.0f);
    g.setColour(ff360_labs::HairlineBorder);
    g.drawRoundedRectangle(barBounds.reduced(0.5f), 2.0f, 1.0f);

    float cx = barBounds.getCentreX();
    float halfWidth = barBounds.getWidth() * 0.5f;
    float indicatorWidth = std::abs(c) * halfWidth;

    juce::Rectangle<float> fillRect;
    juce::Colour fillCol;

    if (c >= 0.0f)
    {
        fillRect = juce::Rectangle<float>(cx, barBounds.getY() + 1.0f, indicatorWidth, barBounds.getHeight() - 2.0f);
        fillCol = ff360_labs::AccentGold;
    }
    else
    {
        fillRect = juce::Rectangle<float>(cx - indicatorWidth, barBounds.getY() + 1.0f, indicatorWidth, barBounds.getHeight() - 2.0f);
        fillCol = ff360_labs::AccentAmberRed;
    }

    g.setColour(fillCol);
    g.fillRoundedRectangle(fillRect, 1.5f);

    // Center Zero Line
    g.setColour(ff360_labs::TextOffWhite.withAlpha(0.5f));
    g.drawLine(cx, barBounds.getY() - 1.0f, cx, barBounds.getBottom() + 1.0f, 1.0f);

    // Bottom Scale Labels (-1, 0, +1)
    g.setFont(FF360LabsLookAndFeel::getCustomFont(8.0f, juce::Font::plain));
    g.setColour(ff360_labs::TextMuted);
    g.drawText("-1", inner.removeFromLeft(20.0f), juce::Justification::centredLeft, false);
    g.drawText("+1", inner.removeFromRight(20.0f), juce::Justification::centredRight, false);
    g.drawText("0", inner, juce::Justification::centred, false);
}

void PhaseScopeModule::paintModule(juce::Graphics& g)
{
    auto bounds = getModuleBounds().toFloat().reduced(6.0f);
    
    auto corrBounds = bounds.removeFromBottom(42.0f);
    bounds.removeFromBottom(6.0f); // Gap

    // Draw Polar Scope Glass Panel
    FF360LabsLookAndFeel::drawGlassPanel(g, bounds, 6.0f);

    updateScopeImage(bounds);
    
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();
    float radius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.44f;

    // Draw Polar Graticule Circles & Guidelines
    g.setColour(ff360_labs::HairlineBorder);
    g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.0f);
    g.drawEllipse(cx - radius * 0.5f, cy - radius * 0.5f, radius, radius, 0.8f);

    // Crosshairs (M/S Axis)
    g.setColour(ff360_labs::TextMuted.withAlpha(0.18f));
    g.drawLine(cx, bounds.getY() + 8.0f, cx, bounds.getBottom() - 8.0f, 1.0f);
    g.drawLine(bounds.getX() + 8.0f, cy, bounds.getRight() - 8.0f, cy, 1.0f);

    // Diagonal 45-degree L / R Axis Guides
    float d = radius * 0.7071f;
    g.drawLine(cx - d, cy + d, cx + d, cy - d, 0.8f);
    g.drawLine(cx + d, cy + d, cx - d, cy - d, 0.8f);

    // Polar Axis Labels (M, S, L, R)
    g.setFont(FF360LabsLookAndFeel::getCustomFont(9.0f, juce::Font::bold));
    g.setColour(ff360_labs::TextMuted);
    g.drawText("+M", juce::Rectangle<float>(cx - 10.0f, bounds.getY() + 3.0f, 20.0f, 12.0f), juce::Justification::centred, false);
    g.drawText("+S", juce::Rectangle<float>(bounds.getRight() - 18.0f, cy - 6.0f, 16.0f, 12.0f), juce::Justification::centred, false);
    g.drawText("-S", juce::Rectangle<float>(bounds.getX() + 3.0f, cy - 6.0f, 16.0f, 12.0f), juce::Justification::centred, false);
    g.drawText("L",  juce::Rectangle<float>(cx - d - 12.0f, cy - d - 6.0f, 14.0f, 12.0f), juce::Justification::centred, false);
    g.drawText("R",  juce::Rectangle<float>(cx + d, cy - d - 6.0f, 14.0f, 12.0f), juce::Justification::centred, false);

    // Draw Persisted Lissajous Scope Trace
    g.drawImage(scopeImage, bounds, juce::RectanglePlacement::stretchToFit, false);
    
    // Draw Correlation Meter Card
    drawCorrelationMeter(g, corrBounds);
}

void PhaseScopeModule::resizedModule()
{
}

