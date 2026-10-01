#include "SpectrumAnalyzerModule.h"
#include "../../Core/Constants.h"
#include "../LookAndFeel/FF360LabsLookAndFeel.h"
#include <cmath>

SpectrumAnalyzerModule::SpectrumAnalyzerModule(AudioFifo<SpectrumData, 8>& fifoToUse, SpectrumDSP& dspToUse, juce::AudioProcessorValueTreeState& apvtsToUse)
    : MeterModule("SPECTRUM ANALYZER", MeterModuleType::Spectrum),
      meterFifo(fifoToUse),
      spectrumDSP(dspToUse),
      apvts(apvtsToUse)
{
    setTooltip("Spectrum Analyzer: level per frequency in dBFS. The low end is interpolated between "
               "FFT bins and the top end averaged over 1/6 octave. Gold is the left channel, grey the right.");

    // Phase 10.5: FFT resolution selector with tooltip explaining the tradeoff
    fftResolutionCombo.setTextWhenNothingSelected("FFT Res");
    fftResolutionCombo.addItem("Low (1024)",    1);
    fftResolutionCombo.addItem("Medium (2048)", 2);
    fftResolutionCombo.addItem("High (4096)",   3);
    fftResolutionCombo.setSelectedId(static_cast<int>(spectrumDSP.getFFTResolution()) - static_cast<int>(FFTResolution::Low) + 1,
                                     juce::dontSendNotification);
    fftResolutionCombo.setTooltip("FFT resolution: more points resolve the low end more finely, "
                                   "but update more slowly and use more CPU.");
    fftResolutionCombo.onChange = [this]
    {
        const int order = static_cast<int>(FFTResolution::Low) + fftResolutionCombo.getSelectedId() - 1;
        spectrumDSP.setFFTResolution(static_cast<FFTResolution>(order));
        apvts.state.setProperty("fftOrder", order, nullptr);
    };
    addAndMakeVisible(fftResolutionCombo);

    startTimerHz(60);
}

SpectrumAnalyzerModule::~SpectrumAnalyzerModule()
{
    stopTimer();
}

void SpectrumAnalyzerModule::timerCallback()
{
    if (meterFifo.pullLatest(currentData) && currentData.numBins > 1)
    {
        currentSampleRate = currentData.sampleRate > 0.0 ? currentData.sampleRate : 48000.0;

        if (currentData.numBins != mappedBins || currentSampleRate != mappedSampleRate)
            rebuildDisplayMap(currentData.numBins, currentSampleRate);

        mapToDisplay(currentData.magnitudesL.data(), currentData.numBins, displayL.data());
        if (currentData.hasRight)
            mapToDisplay(currentData.magnitudesR.data(), currentData.numBins, displayR.data());

        repaint(getModuleBounds());
    }
}

void SpectrumAnalyzerModule::rebuildDisplayMap(int numBins, double sampleRate)
{
    // Display points are log-spaced from 20 Hz to Nyquist, matching the grid
    const double binWidth = sampleRate / (2.0 * numBins);
    const double minFreq = 20.0;
    const double maxFreq = sampleRate * 0.5;
    const double ratio = std::pow(maxFreq / minFreq, 1.0 / (double) (NumDisplayPoints - 1));
    const double halfBand = std::pow(2.0, 0.5 * SmoothingOctaves);
    const double maxPos = (double) numBins - 0.5;

    for (int i = 0; i < NumDisplayPoints; ++i)
    {
        auto& p = displayPoints[(size_t) i];
        const double centre = minFreq * std::pow(ratio, (double) i) / binWidth;

        // Below the first bin, hold its level: bin 0 is DC, and the old trace sloped down to
        // the corner there, which looked like the low end vanishing at Low (1024)
        p.centre = (float) juce::jlimit(1.0, (double) (numBins - 1), centre);

        // Triangle peaking at the centre, zero at the band edges (log-symmetric)
        const double lo = juce::jlimit(1.0, maxPos, centre / halfBand);
        const double hi = juce::jlimit(1.0, maxPos, centre * halfBand);
        p.lo = (float) lo;
        p.hi = (float) hi;
        p.invRise = (float) (1.0 / std::max(1.0e-6, p.centre - lo));
        p.invFall = (float) (1.0 / std::max(1.0e-6, hi - p.centre));
        p.k0 = juce::jlimit(1, numBins - 1, (int) std::ceil(lo));
        p.k1 = juce::jlimit(1, numBins - 1, (int) std::floor(hi));

        double totalW = 0.0;
        for (int k = p.k0; k <= p.k1; ++k)
            totalW += p.weight(k);
        p.invTotalW = totalW > 1.0e-6 ? (float) (1.0 / totalW) : 0.0f;

        // 0 (interpolate) while the band covers under ~2 bins, 1 (average) once it covers ~4
        p.bandBlend = totalW > 1.0e-6 ? (float) juce::jlimit(0.0, 1.0, 0.5 * (hi - lo) - 1.0) : 0.0f;
    }

    mappedBins = numBins;
    mappedSampleRate = sampleRate;
}

void SpectrumAnalyzerModule::mapToDisplay(const float* binsDb, int numBins, float* outDb) const
{
    auto dbToPower = [](float db) { return std::pow(10.0f, db * 0.1f); };

    for (int i = 0; i < NumDisplayPoints; ++i)
    {
        const auto& p = displayPoints[(size_t) i];

        // Sparse bins (low end): Catmull-Rom through the bin levels, so the curve is smooth
        // instead of a straight segment per bin
        float db = 0.0f;
        if (p.bandBlend < 1.0f)
        {
            const int k = std::min((int) p.centre, numBins - 2);
            const float t = p.centre - (float) k;
            const float y0 = binsDb[std::max(1, k - 1)];
            const float y1 = binsDb[k];
            const float y2 = binsDb[k + 1];
            const float y3 = binsDb[std::min(numBins - 1, k + 2)];
            db = y1 + 0.5f * t * ((y2 - y0) + t * ((2.0f * y0 - 5.0f * y1 + 4.0f * y2 - y3) + t * (3.0f * (y1 - y2) + y3 - y0)));
        }

        // Dense bins (top end): triangular-weighted mean power over the band, so it isn't jagged
        if (p.bandBlend > 0.0f)
        {
            float sum = 0.0f;
            for (int k = p.k0; k <= p.k1; ++k)
                sum += dbToPower(binsDb[k]) * p.weight(k);
            const float bandDb = 10.0f * std::log10(std::max(1.0e-12f, sum * p.invTotalW));
            db += p.bandBlend * (bandDb - db);
        }

        outDb[i] = juce::jlimit(-120.0f, 12.0f, db);
    }
}

void SpectrumAnalyzerModule::drawSpectrum(juce::Graphics& g,
                                           const float* displayDb,
                                           juce::Rectangle<float> plotArea,
                                           float minDb, float rangeDb,
                                           juce::Colour lineColour,
                                           bool drawFill)
{
    float w = plotArea.getWidth();
    float h = plotArea.getHeight();
    float yOffset = plotArea.getY();
    float xOffset = plotArea.getX();

    juce::Path curvePath;
    juce::Path fillPath;

    for (int i = 0; i < NumDisplayPoints; ++i)
    {
        float normalizedY = 1.0f - juce::jlimit(0.0f, 1.0f, (displayDb[i] - minDb) / rangeDb);

        // Points are log-spaced across the plot
        float x = xOffset + w * (float) i / (float) (NumDisplayPoints - 1);
        float y = yOffset + (normalizedY * h);

        if (i == 0)
        {
            fillPath.startNewSubPath(x, yOffset + h);
            fillPath.lineTo(x, y);
            curvePath.startNewSubPath(x, y);
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

    if (mappedBins < 2) return;

    // 3. Draw R channel first (behind), desaturated gold-gray, no fill
    if (currentData.hasRight)
    {
        juce::Colour rColour = ff360_labs::AccentGold
                                   .withSaturation(0.25f)
                                   .withAlpha(0.55f);
        drawSpectrum(g, displayR.data(), plotArea, minDb, rangeDb, rColour, false);
    }

    // 4. Draw L channel on top, full gold with gradient fill
    drawSpectrum(g, displayL.data(), plotArea, minDb, rangeDb,
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
