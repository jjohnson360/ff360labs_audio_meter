#include "SpectrumAnalyzerModule.h"
#include "../../Core/Constants.h"
#include "../LookAndFeel/FF360LabsLookAndFeel.h"
#include <cmath>

SpectrumAnalyzerModule::SpectrumAnalyzerModule(AudioFifo<SpectrumData>& fifoToUse, double sampleRate)
    : MeterModule("SPECTRUM ANALYZER", MeterModuleType::Spectrum), 
      meterFifo(fifoToUse),
      currentSampleRate(sampleRate > 0.0 ? sampleRate : 48000.0)
{
    startTimerHz(60);
}

SpectrumAnalyzerModule::~SpectrumAnalyzerModule()
{
    stopTimer();
}

void SpectrumAnalyzerModule::timerCallback()
{
    SpectrumData newData;
    bool hasNewData = false;
    
    while (meterFifo.pull(newData))
    {
        currentData = newData;
        hasNewData = true;
    }
    
    if (hasNewData && !currentData.magnitudes.empty())
    {
        repaint();
    }
}

float SpectrumAnalyzerModule::getLogX(float binIndex, float numBins, float width)
{
    float minFreq = 20.0f;
    float maxFreq = static_cast<float>(currentSampleRate) / 2.0f;
    
    float binFreq = (binIndex / numBins) * maxFreq;
    binFreq = std::max(minFreq, binFreq);
    
    float logMin = std::log10(minFreq);
    float logMax = std::log10(maxFreq);
    float logFreq = std::log10(binFreq);
    
    float normalized = (logFreq - logMin) / (logMax - logMin);
    return juce::jlimit(0.0f, 1.0f, normalized) * width;
}

void SpectrumAnalyzerModule::paintModule(juce::Graphics& g)
{
    auto bounds = getModuleBounds().toFloat().reduced(6.0f);
    
    // Draw Glass Panel
    FF360LabsLookAndFeel::drawGlassPanel(g, bounds, 6.0f);

    auto plotArea = bounds.reduced(8.0f);
    auto bottomAxis = plotArea.removeFromBottom(14.0f);

    float w = plotArea.getWidth();
    float h = plotArea.getHeight();
    float yOffset = plotArea.getY();
    float xOffset = plotArea.getX();

    // 1. Draw Subtle Log Frequency Vertical Gridlines
    const struct FreqLabel { float freq; const char* text; bool showText; } freqGrid[] = {
        { 20.0f, "20", true },
        { 50.0f, "50", false },
        { 100.0f, "100", true },
        { 250.0f, "250", false },
        { 500.0f, "500", false },
        { 1000.0f, "1k", true },
        { 2500.0f, "2.5k", false },
        { 5000.0f, "5k", true },
        { 10000.0f, "10k", true },
        { 20000.0f, "20k", true }
    };

    float minFreq = 20.0f;
    float maxFreq = static_cast<float>(currentSampleRate) / 2.0f;
    float logMin = std::log10(minFreq);
    float logMax = std::log10(maxFreq);

    g.setFont(FF360LabsLookAndFeel::getCustomFont(8.5f, juce::Font::plain));

    for (const auto& item : freqGrid)
    {
        if (item.freq > maxFreq) continue;
        float logF = std::log10(item.freq);
        float normX = (logF - logMin) / (logMax - logMin);
        float gx = xOffset + (normX * w);

        // Vertical hairline gridline
        g.setColour(ff360_labs::HairlineBorder.withAlpha(item.showText ? 0.20f : 0.08f));
        g.drawLine(gx, yOffset, gx, yOffset + h, 1.0f);

        // Bottom axis label
        if (item.showText)
        {
            g.setColour(ff360_labs::TextMuted);
            g.drawText(item.text, juce::Rectangle<float>(gx - 15.0f, bottomAxis.getY() + 2.0f, 30.0f, 12.0f),
                       juce::Justification::centred, false);
        }
    }

    // 2. Horizontal dB Gridlines (-12, -24, -48, -72 dB)
    const float dbTicks[] = { 0.0f, -12.0f, -24.0f, -48.0f, -72.0f };
    float minDb = -80.0f;
    float maxDb = 0.0f;
    float rangeDb = maxDb - minDb;

    for (float db : dbTicks)
    {
        float normY = 1.0f - (db - minDb) / rangeDb;
        float gy = yOffset + normY * h;

        g.setColour(ff360_labs::HairlineBorder.withAlpha(0.12f));
        g.drawLine(xOffset, gy, xOffset + w, gy, 1.0f);

        g.setColour(ff360_labs::TextMuted.withAlpha(0.6f));
        g.drawText(juce::String((int)db), juce::Rectangle<float>(xOffset + w - 24.0f, gy - 6.0f, 22.0f, 12.0f),
                   juce::Justification::centredRight, false);
    }

    if (currentData.magnitudes.empty()) return;

    int numBins = static_cast<int>(currentData.magnitudes.size());

    // 3. Build Smooth Spectrum Curve Path
    juce::Path curvePath;
    juce::Path fillPath;
    bool first = true;

    for (int i = 1; i < numBins; ++i)
    {
        float db = currentData.magnitudes[i];
        float normalizedY = 1.0f - juce::jlimit(0.0f, 1.0f, (db - minDb) / rangeDb);
        
        float x = xOffset + getLogX(static_cast<float>(i), static_cast<float>(numBins), w);
        float y = yOffset + (normalizedY * h);
        
        if (first)
        {
            fillPath.startNewSubPath(xOffset, yOffset + h);
            fillPath.lineTo(x, y);
            curvePath.startNewSubPath(x, y);
            first = false;
        }
        else
        {
            fillPath.lineTo(x, y);
            curvePath.lineTo(x, y);
        }
    }

    fillPath.lineTo(xOffset + w, yOffset + h);
    fillPath.closeSubPath();

    // 4. Fill with Metallic Gold to Amber-Red Gradient
    juce::ColourGradient gradient(ff360_labs::AccentAmberRed.withAlpha(0.60f), xOffset, yOffset,
                                  ff360_labs::AccentGold.withAlpha(0.12f), xOffset, yOffset + h,
                                  false);
    gradient.addColour(0.35f, ff360_labs::AccentGold.withAlpha(0.35f));

    g.setGradientFill(gradient);
    g.fillPath(fillPath);

    // 5. Glowing Contour Outline
    g.setColour(ff360_labs::GoldGlow);
    g.strokePath(curvePath, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour(ff360_labs::AccentGold);
    g.strokePath(curvePath, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void SpectrumAnalyzerModule::resizedModule()
{
}

