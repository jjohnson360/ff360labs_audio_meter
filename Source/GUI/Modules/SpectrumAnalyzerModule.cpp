#include "SpectrumAnalyzerModule.h"
#include "../../Core/Constants.h"
#include "../LookAndFeel/FF360LabsLookAndFeel.h"
#include <cmath>

SpectrumAnalyzerModule::SpectrumAnalyzerModule(AudioFifo<SpectrumData>& fifoToUse, double sampleRate)
    : MeterModule("SPECTRUM ANALYZER", MeterModuleType::Spectrum),
      meterFifo(fifoToUse),
      currentSampleRate(sampleRate > 0.0 ? sampleRate : 48000.0)
{
    // Phase 10.5: FFT resolution selector with tooltip explaining the tradeoff
    fftResolutionCombo.setTextWhenNothingSelected("FFT Res");
    fftResolutionCombo.addItem("Low (1024)",    1);
    fftResolutionCombo.addItem("Medium (2048)", 2);
    fftResolutionCombo.addItem("High (4096)",   3);
    fftResolutionCombo.setSelectedId(2, juce::dontSendNotification); // Medium default
    fftResolutionCombo.setTooltip("FFT Resolution: higher orders give better frequency resolution "
                                   "at the cost of increased CPU usage and update latency.");
    addAndMakeVisible(fftResolutionCombo);

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

    if (hasNewData && !currentData.magnitudesL.empty())
        repaint();
}

float SpectrumAnalyzerModule::getLogX(float binIndex, float numBins, float width)
{
    float minFreq = 20.0f;
    float maxFreq = static_cast<float>(currentSampleRate) / 2.0f;

    float binFreq = (binIndex / numBins) * maxFreq;
    binFreq = std::max(minFreq, binFreq);

    float logMin  = std::log10(minFreq);
    float logMax  = std::log10(maxFreq);
    float logFreq = std::log10(binFreq);

    float normalized = (logFreq - logMin) / (logMax - logMin);
    return juce::jlimit(0.0f, 1.0f, normalized) * width;
}

void SpectrumAnalyzerModule::drawSpectrum(juce::Graphics& g,
                                           const std::vector<float>& magnitudes,
                                           juce::Rectangle<float> plotArea,
                                           float minDb, float rangeDb,
                                           juce::Colour lineColour,
                                           bool drawFill)
{
    float w = plotArea.getWidth();
    float h = plotArea.getHeight();
    float yOffset = plotArea.getY();
    float xOffset = plotArea.getX();

    int numBins = static_cast<int>(magnitudes.size());
    if (numBins < 2) return;

    juce::Path curvePath;
    juce::Path fillPath;
    bool first = true;

    for (int i = 1; i < numBins; ++i)
    {
        float db = magnitudes[static_cast<size_t>(i)];
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

    if (drawFill)
    {
        // Gold-to-amber gradient fill for L channel (primary trace)
        juce::ColourGradient gradient(ff360_labs::AccentAmberRed.withAlpha(0.55f), xOffset, yOffset,
                                      ff360_labs::AccentGold.withAlpha(0.10f),   xOffset, yOffset + h,
                                      false);
        gradient.addColour(0.35f, ff360_labs::AccentGold.withAlpha(0.30f));
        g.setGradientFill(gradient);
        g.fillPath(fillPath);
    }

    // Glow halo
    g.setColour(lineColour.withAlpha(0.25f));
    g.strokePath(curvePath, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Sharp contour
    g.setColour(lineColour);
    g.strokePath(curvePath, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void SpectrumAnalyzerModule::paintModule(juce::Graphics& g)
{
    auto bounds = getModuleBounds().toFloat().reduced(6.0f);

    // Top strip for resolution selector
    auto topStrip = bounds.removeFromTop(24.0f).reduced(2.0f, 2.0f);
    // (fftResolutionCombo is laid out in resizedModule)

    // Draw Glass Panel
    FF360LabsLookAndFeel::drawGlassPanel(g, bounds, 6.0f);

    auto plotArea = bounds.reduced(8.0f);
    auto bottomAxis = plotArea.removeFromBottom(14.0f);

    float w = plotArea.getWidth();
    float h = plotArea.getHeight();
    float yOffset = plotArea.getY();
    float xOffset = plotArea.getX();

    // 1. Log Frequency Vertical Gridlines
    const struct FreqLabel { float freq; const char* text; bool showText; } freqGrid[] = {
        { 20.0f,    "20",  true  },
        { 50.0f,    "50",  false },
        { 100.0f,   "100", true  },
        { 250.0f,   "250", false },
        { 500.0f,   "500", false },
        { 1000.0f,  "1k",  true  },
        { 2500.0f,  "2.5k",false },
        { 5000.0f,  "5k",  true  },
        { 10000.0f, "10k", true  },
        { 20000.0f, "20k", true  }
    };

    float minFreq = 20.0f;
    float maxFreq = static_cast<float>(currentSampleRate) / 2.0f;
    float logMin  = std::log10(minFreq);
    float logMax  = std::log10(maxFreq);

    g.setFont(FF360LabsLookAndFeel::getCustomFont(8.5f, juce::Font::plain));

    for (const auto& item : freqGrid)
    {
        if (item.freq > maxFreq) continue;
        float logF = std::log10(item.freq);
        float normX = (logF - logMin) / (logMax - logMin);
        float gx = xOffset + (normX * w);

        g.setColour(ff360_labs::HairlineBorder.withAlpha(item.showText ? 0.20f : 0.08f));
        g.drawLine(gx, yOffset, gx, yOffset + h, 1.0f);

        if (item.showText)
        {
            g.setColour(ff360_labs::TextMuted);
            g.drawText(item.text,
                       juce::Rectangle<float>(gx - 15.0f, bottomAxis.getY() + 2.0f, 30.0f, 12.0f),
                       juce::Justification::centred, false);
        }
    }

    // 2. Horizontal dB Gridlines
    const float dbTicks[] = { 0.0f, -12.0f, -24.0f, -48.0f, -72.0f };
    float minDb  = -80.0f;
    float maxDb  =   0.0f;
    float rangeDb = maxDb - minDb;

    for (float db : dbTicks)
    {
        float normY = 1.0f - (db - minDb) / rangeDb;
        float gy = yOffset + normY * h;

        g.setColour(ff360_labs::HairlineBorder.withAlpha(0.12f));
        g.drawLine(xOffset, gy, xOffset + w, gy, 1.0f);

        g.setColour(ff360_labs::TextMuted.withAlpha(0.6f));
        g.drawText(juce::String((int)db),
                   juce::Rectangle<float>(xOffset + w - 24.0f, gy - 6.0f, 22.0f, 12.0f),
                   juce::Justification::centredRight, false);
    }

    if (currentData.magnitudesL.empty()) return;

    // 3. Draw R channel first (behind), desaturated gold-gray, no fill
    if (!currentData.magnitudesR.empty())
    {
        juce::Colour rColour = ff360_labs::AccentGold
                                   .withSaturation(0.25f)
                                   .withAlpha(0.55f);
        drawSpectrum(g, currentData.magnitudesR, plotArea, minDb, rangeDb, rColour, false);
    }

    // 4. Draw L channel on top, full gold with gradient fill
    drawSpectrum(g, currentData.magnitudesL, plotArea, minDb, rangeDb,
                 ff360_labs::AccentGold, true);

    // 5. Channel legend (top-right corner)
    g.setFont(FF360LabsLookAndFeel::getCustomFont(8.0f, juce::Font::plain));
    auto legendArea = plotArea.withLeft(plotArea.getRight() - 40.0f).withHeight(22.0f);
    g.setColour(ff360_labs::AccentGold);
    g.drawText("L", juce::Rectangle<float>(legendArea.getX(), legendArea.getY(), 12.0f, 10.0f),
               juce::Justification::left, false);
    g.setColour(ff360_labs::AccentGold.withSaturation(0.25f).withAlpha(0.55f));
    g.drawText("R", juce::Rectangle<float>(legendArea.getX() + 14.0f, legendArea.getY(), 12.0f, 10.0f),
               juce::Justification::left, false);
}

void SpectrumAnalyzerModule::resizedModule()
{
    auto bounds = getModuleBounds().reduced(4);
    auto topStrip = bounds.removeFromTop(24);
    fftResolutionCombo.setBounds(topStrip.removeFromLeft(130).reduced(0, 2));
}
