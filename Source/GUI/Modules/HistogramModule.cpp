#include "HistogramModule.h"
#include "../../Core/Constants.h"
#include "../LookAndFeel/FF360LabsLookAndFeel.h"

HistogramModule::HistogramModule(AudioFifo<HistogramData>& fifoToUse, std::function<void()> onResetCallback)
    : MeterModule("HISTOGRAM (5 MIN)", MeterModuleType::Histogram), 
      meterFifo(fifoToUse),
      onReset(onResetCallback)
{
    addAndMakeVisible(btnCaptureA);
    addAndMakeVisible(btnToggleAB);
    addAndMakeVisible(btnReset);

    btnToggleAB.setEnabled(false);

    btnCaptureA.onClick = [this]
    {
        snapshotA = currentData;
        hasSnapshotA = true;
        btnToggleAB.setEnabled(true);
        btnCaptureA.setButtonText("A CAPTURED");
        repaint();
    };

    btnToggleAB.onClick = [this]
    {
        isComparing = !isComparing;
        btnToggleAB.setButtonText(isComparing ? "A/B: ON" : "A/B: OFF");
        repaint();
    };

    btnReset.onClick = [this] 
    { 
        if (onReset) onReset(); 
        currentData.bins.fill(0);
        currentData.maxCount = 0;
        currentData.modalBinIndex = 0;
        repaint();
    };

    startTimerHz(15);
}

HistogramModule::~HistogramModule()
{
    stopTimer();
}

void HistogramModule::timerCallback()
{
    HistogramData newData;
    bool hasNewData = false;
    
    while (meterFifo.pull(newData))
    {
        currentData = newData;
        hasNewData = true;
    }
    
    if (hasNewData)
    {
        repaint();
    }
}

void HistogramModule::paintModule(juce::Graphics& g)
{
    auto bounds = getModuleBounds().toFloat().reduced(6.0f);
    
    // Draw Glass Panel
    FF360LabsLookAndFeel::drawGlassPanel(g, bounds, 6.0f);

    auto inner = bounds.reduced(10.0f, 8.0f);
    
    float w = inner.getWidth();
    float h = inner.getHeight();
    float yOffset = inner.getY();
    float xOffset = inner.getX();
    
    // The bins represent -70 to 0 LUFS. We display -60 to 0.
    int minBin = 10; // -60 LUFS
    int maxBin = 70; // 0 LUFS
    int numDisplayedBins = maxBin - minBin + 1;
    
    float barHeight = h / (float)numDisplayedBins;
    float labelWidth = 32.0f;
    
    g.setFont(FF360LabsLookAndFeel::getCustomFont(8.5f, juce::Font::plain));

    // A/B Delta Readout Badge if comparing
    if (isComparing && hasSnapshotA && currentData.maxCount > 0 && snapshotA.maxCount > 0)
    {
        float modalA_Lufs = (float)(snapshotA.modalBinIndex - 70);
        float modalB_Lufs = (float)(currentData.modalBinIndex - 70);
        float deltaLufs = modalB_Lufs - modalA_Lufs;

        juce::String deltaStr = juce::String("A/B MODAL Δ: ") + (deltaLufs > 0 ? "+" : "") + juce::String(deltaLufs, 1) + " LUFS";
        
        g.setFont(FF360LabsLookAndFeel::getCustomFont(8.5f, juce::Font::bold));
        g.setColour(juce::Colour(0xFF7DD3FC));
        g.drawText(deltaStr, juce::Rectangle<float>(xOffset + labelWidth + 10.0f, yOffset - 4.0f, 180.0f, 16.0f),
                   juce::Justification::centredLeft, false);
    }
    
    for (int i = minBin; i <= maxBin; ++i)
    {
        int countB = currentData.bins[i];
        int countA = (hasSnapshotA && isComparing) ? snapshotA.bins[i] : 0;
        
        float normalizedWidthB = (currentData.maxCount > 0) ? (static_cast<float>(countB) / static_cast<float>(currentData.maxCount)) : 0.0f;
        float normalizedWidthA = (hasSnapshotA && snapshotA.maxCount > 0) ? (static_cast<float>(countA) / static_cast<float>(snapshotA.maxCount)) : 0.0f;
        
        float barWB = normalizedWidthB * (w - labelWidth);
        float barWA = normalizedWidthA * (w - labelWidth);
        
        // Draw from bottom to top (0 LUFS at top, -60 at bottom)
        int displayIndex = maxBin - i;
        float barY = yOffset + (displayIndex * barHeight);
        
        juce::Rectangle<float> barRectB(xOffset + labelWidth, barY + 0.5f, barWB, barHeight - 1.0f);
        juce::Rectangle<float> barRectA(xOffset + labelWidth, barY + 0.5f, barWA, barHeight - 1.0f);
        
        // 1. Draw Snapshot A overlay if active (Cool Slate Cyan)
        if (isComparing && hasSnapshotA && barWA > 1.0f)
        {
            bool isModalA = (i == snapshotA.modalBinIndex && countA > 0);
            if (isModalA)
            {
                g.setColour(juce::Colour(0xFF7DD3FC).withAlpha(0.75f));
                g.drawRoundedRectangle(barRectA.expanded(1.0f, 0.5f), 2.0f, 1.2f);
                g.setColour(juce::Colour(0xFF7DD3FC).withAlpha(0.45f));
                g.fillRoundedRectangle(barRectA, 2.0f);
            }
            else
            {
                g.setColour(juce::Colour(0xFF7DD3FC).withAlpha(0.20f));
                g.fillRoundedRectangle(barRectA, 1.5f);
            }
        }

        // 2. Draw Live B Bars (Metallic Gold)
        bool isModalB = (i == currentData.modalBinIndex && countB > 0);
        if (barWB > 1.0f)
        {
            if (isModalB)
            {
                // Modal Bucket Glow + Solid Gold Fill
                g.setColour(ff360_labs::GoldGlow);
                g.drawRoundedRectangle(barRectB.expanded(1.0f, 0.5f), 2.0f, 1.5f);

                g.setColour(ff360_labs::AccentGold);
                g.fillRoundedRectangle(barRectB, 2.0f);
            }
            else
            {
                // Dimmer Charcoal-Gold Tint
                g.setColour(ff360_labs::AccentGold.withAlpha(isComparing ? 0.38f : 0.28f));
                g.fillRoundedRectangle(barRectB, 1.5f);
            }
        }
        
        // Draw labels every 10 LUFS
        if (i % 10 == 0)
        {
            g.setColour(ff360_labs::TextMuted);
            juce::String labelStr;
            labelStr << (i - 70);
            g.drawText(labelStr, juce::Rectangle<float>(xOffset, barY - 2.0f, labelWidth - 4.0f, barHeight + 4.0f),
                       juce::Justification::centredRight, false);

            // Subtle horizontal reference guide
            g.setColour(ff360_labs::HairlineBorder.withAlpha(0.12f));
            g.drawHorizontalLine((int)(barY + barHeight * 0.5f), xOffset + labelWidth, xOffset + w);
        }
    }
}

void HistogramModule::resizedModule()
{
    auto bounds = getLocalBounds();
    int right = bounds.getRight() - 52;
    
    btnReset.setBounds(right, 3, 44, 18);
    right -= 56;
    btnToggleAB.setBounds(right, 3, 52, 18);
    right -= 70;
    btnCaptureA.setBounds(right, 3, 66, 18);
}

