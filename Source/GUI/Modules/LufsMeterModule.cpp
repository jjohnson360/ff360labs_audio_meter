#include "LufsMeterModule.h"
#include "../../Core/Constants.h"
#include "../LookAndFeel/FF360LabsLookAndFeel.h"

LufsMeterModule::LufsMeterModule(AudioFifo<LufsMeterData>& fifoToUse, LufsDSP& dspInstance, juce::AudioProcessorValueTreeState* apvts)
    : MeterModule("LUFS METER", MeterModuleType::LUFS), meterFifo(fifoToUse), lufsDSP(dspInstance)
{
    const auto& presets = ff360_labs::LoudnessTarget::getBuiltinPresets();
    for (size_t i = 0; i < presets.size(); ++i)
    {
        targetSelector.addItem(presets[i].name, (int)i + 1);
    }
    
    if (apvts != nullptr)
    {
        targetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(*apvts, "targetProfile", targetSelector);
    }
    else
    {
        targetSelector.setSelectedId(1);
    }
    
    currentTargetProfile = ff360_labs::LoudnessTarget::getPresetByIndex(targetSelector.getSelectedItemIndex());
    addAndMakeVisible(targetSelector);
    
    targetSelector.onChange = [this] {
        int idx = targetSelector.getSelectedItemIndex();
        currentTargetProfile = ff360_labs::LoudnessTarget::getPresetByIndex(idx >= 0 ? idx : 0);
        repaint();
    };
    
    addAndMakeVisible(resetButton);
    resetButton.onClick = [this] {
        lufsDSP.reset();
        currentData = LufsMeterData();
        momentaryDamper.reset(-60.0f);
        shortTermDamper.reset(-60.0f);
        integratedDamper.reset(-60.0f);
        repaint();
    };
    
    startTimerHz(60);
}

LufsMeterModule::~LufsMeterModule()
{
    stopTimer();
}

void LufsMeterModule::timerCallback()
{
    LufsMeterData newData;
    if (meterFifo.pullLatest(newData))
    {
        currentData = newData;
    }

    float mVal = juce::jlimit(-60.0f, 0.0f, currentData.momentary <= -69.0f ? -60.0f : currentData.momentary);
    float sVal = juce::jlimit(-60.0f, 0.0f, currentData.shortTerm <= -69.0f ? -60.0f : currentData.shortTerm);
    float iVal = juce::jlimit(-60.0f, 0.0f, currentData.integrated <= -69.0f ? -60.0f : currentData.integrated);

    momentaryDamper.setTarget(mVal);
    shortTermDamper.setTarget(sVal);
    integratedDamper.setTarget(iVal);

    momentaryDamper.update(1.0f / 60.0f);
    shortTermDamper.update(1.0f / 60.0f);
    integratedDamper.update(1.0f / 60.0f);

    repaint();
}

void LufsMeterModule::drawLufsDial (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    FF360LabsLookAndFeel::drawGlassPanel(g, bounds, 6.0f);

    auto inner = bounds.reduced(8.0f);
    float size = juce::jmin(inner.getWidth(), inner.getHeight());
    juce::Rectangle<float> dialArea(inner.getCentreX() - size * 0.5f, inner.getCentreY() - size * 0.5f, size, size);

    float cx = dialArea.getCentreX();
    float cy = dialArea.getCentreY();
    float radius = size * 0.44f;

    // Angle range: -130 deg to +130 deg (260 deg total arc)
    float minAngle = -juce::MathConstants<float>::pi * 0.72f;
    float maxAngle =  juce::MathConstants<float>::pi * 0.72f;

    float minLufs = -54.0f;
    float maxLufs = 0.0f;

    auto lufsToAngle = [&](float lufs) {
        float norm = juce::jlimit(0.0f, 1.0f, (lufs - minLufs) / (maxLufs - minLufs));
        return minAngle + norm * (maxAngle - minAngle);
    };

    // 1. Background Arc Track
    float trackThickness = 10.0f;
    juce::Path bgArc;
    bgArc.addCentredArc(cx, cy, radius, radius, 0.0f, minAngle, maxAngle, true);
    g.setColour(ff360_labs::ContainerDark.darker(0.5f));
    g.strokePath(bgArc, juce::PathStrokeType(trackThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(ff360_labs::HairlineBorder);
    g.strokePath(bgArc, juce::PathStrokeType(1.0f));

    // 2. Target Tolerance Zone Arc Bracket
    float tLufs = currentTargetProfile.targetLufs;
    float tol = currentTargetProfile.tolerance;
    float tolMinAngle = lufsToAngle(tLufs - tol);
    float tolMaxAngle = lufsToAngle(tLufs + tol);

    juce::Path tolArc;
    tolArc.addCentredArc(cx, cy, radius, radius, 0.0f, tolMinAngle, tolMaxAngle, true);
    g.setColour(ff360_labs::AccentGold.withAlpha(0.25f));
    g.strokePath(tolArc, juce::PathStrokeType(trackThickness + 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Target Exact Reference Marker Line
    float targetAngle = lufsToAngle(tLufs);
    juce::Point<float> targetP1 = juce::Point<float>(cx, cy).getPointOnCircumference(radius - trackThickness * 0.8f, targetAngle);
    juce::Point<float> targetP2 = juce::Point<float>(cx, cy).getPointOnCircumference(radius + trackThickness * 0.8f, targetAngle);
    g.setColour(ff360_labs::AccentGold);
    g.drawLine(juce::Line<float>(targetP1, targetP2), 2.5f);

    // 3. Integrated LUFS Arc (Illuminated Metallic Gold to Amber-Red)
    float curIntegrated = integratedDamper.getCurrent();
    float iAngle = lufsToAngle(curIntegrated);
    if (iAngle > minAngle + 0.05f)
    {
        juce::Path iArc;
        iArc.addCentredArc(cx, cy, radius, radius, 0.0f, minAngle, iAngle, true);

        bool isOverTarget = (curIntegrated > tLufs + tol);

        // Soft glow under fill
        g.setColour(isOverTarget ? ff360_labs::AccentAmberRed.withAlpha(0.25f)
                                 : ff360_labs::GoldGlow);
        g.strokePath(iArc, juce::PathStrokeType(trackThickness + 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Arc fill gradient
        juce::ColourGradient arcGrad(ff360_labs::AccentGold, cx - radius, cy,
                                    ff360_labs::AccentAmberRed, cx + radius, cy - radius, false);
        g.setGradientFill(arcGrad);
        g.strokePath(iArc, juce::PathStrokeType(trackThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 4. Momentary & Short-Term Pointers on Outer Rim
    float mAngle = lufsToAngle(momentaryDamper.getCurrent());
    float sAngle = lufsToAngle(shortTermDamper.getCurrent());

    // Short-term pointer (warm off-white notch)
    juce::Point<float> sNotch = juce::Point<float>(cx, cy).getPointOnCircumference(radius + trackThickness * 0.9f, sAngle);
    g.setColour(ff360_labs::TextOffWhite.withAlpha(0.85f));
    g.fillEllipse(sNotch.x - 3.0f, sNotch.y - 3.0f, 6.0f, 6.0f);

    // Momentary pointer (metallic gold diamond/dot)
    juce::Point<float> mNotch = juce::Point<float>(cx, cy).getPointOnCircumference(radius - trackThickness * 0.9f, mAngle);
    g.setColour(ff360_labs::AccentGold);
    g.fillEllipse(mNotch.x - 2.5f, mNotch.y - 2.5f, 5.0f, 5.0f);

    // 5. Calibration Ticks & Numbers
    const int ticks[] = { -54, -42, -36, -24, -18, -14, -9, 0 };
    g.setFont(FF360LabsLookAndFeel::getCustomFont(8.0f, juce::Font::plain));

    for (int t : ticks)
    {
        float angle = lufsToAngle((float)t);
        juce::Point<float> pIn = juce::Point<float>(cx, cy).getPointOnCircumference(radius - trackThickness * 0.6f, angle);
        juce::Point<float> pOut = juce::Point<float>(cx, cy).getPointOnCircumference(radius + trackThickness * 0.6f, angle);
        
        g.setColour(ff360_labs::HairlineBorder);
        g.drawLine(juce::Line<float>(pIn, pOut), 1.0f);

        juce::Point<float> pText = juce::Point<float>(cx, cy).getPointOnCircumference(radius - trackThickness * 1.5f, angle);
        g.setColour(ff360_labs::TextMuted);
        g.drawText(juce::String(t), juce::Rectangle<float>(pText.x - 12.0f, pText.y - 6.0f, 24.0f, 12.0f),
                   juce::Justification::centred, false);
    }

    // 6. Central Large Digital Readout
    auto centerArea = juce::Rectangle<float>(cx - radius * 0.65f, cy - radius * 0.40f, radius * 1.3f, radius * 0.8f);
    
    g.setFont(FF360LabsLookAndFeel::getCustomFont(10.0f, juce::Font::bold));
    g.setColour(ff360_labs::TextMuted);
    g.drawText("INTEGRATED", centerArea.removeFromTop(14.0f), juce::Justification::centred, false);

    juce::String valStr = (currentData.integrated <= -69.0f) ? "-inf" : juce::String(currentData.integrated, 1);
    float diff = currentData.integrated - tLufs;
    bool isWarning = (currentData.integrated > -69.0f && std::abs(diff) > tol);

    g.setFont(FF360LabsLookAndFeel::getNumericReadoutFont(22.0f));
    g.setColour(FF360LabsLookAndFeel::getReadoutColour(isWarning));
    g.drawText(valStr, centerArea.removeFromTop(24.0f), juce::Justification::centred, false);

    g.setFont(FF360LabsLookAndFeel::getCustomFont(10.0f, juce::Font::plain));
    g.setColour(ff360_labs::TextOffWhite.withAlpha(0.6f));
    g.drawText("LUFS", centerArea, juce::Justification::centred, false);
}

void LufsMeterModule::drawStatCluster (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    float tileH = (bounds.getHeight() - 12.0f) / 4.0f;
    
    // Evaluate target compliance
    float diff = currentData.integrated - currentTargetProfile.targetLufs;
    juce::String badgeI = "";
    bool warnI = false;

    if (currentData.integrated > -69.0f)
    {
        if (std::abs(diff) <= currentTargetProfile.tolerance)
        {
            badgeI = "PASS";
            warnI = false;
        }
        else if (diff > currentTargetProfile.tolerance)
        {
            badgeI = "HIGH";
            warnI = true;
        }
        else
        {
            badgeI = "LOW";
            warnI = true;
        }
    }

    auto rect1 = bounds.removeFromTop(tileH);
    bounds.removeFromTop(4.0f);
    auto rect2 = bounds.removeFromTop(tileH);
    bounds.removeFromTop(4.0f);
    auto rect3 = bounds.removeFromTop(tileH);
    bounds.removeFromTop(4.0f);
    auto rect4 = bounds.removeFromTop(tileH);

    juce::String mStr = (currentData.momentary <= -69.0f) ? "-inf" : juce::String(currentData.momentary, 1) + " LUFS";
    juce::String sStr = (currentData.shortTerm <= -69.0f) ? "-inf" : juce::String(currentData.shortTerm, 1) + " LUFS";
    juce::String iStr = (currentData.integrated <= -69.0f) ? "-inf" : juce::String(currentData.integrated, 1) + " LUFS";
    juce::String lraStr = juce::String(currentData.lra, 1) + " LU";

    FF360LabsLookAndFeel::drawStatTile(g, rect1, "LUFS-I (TARGET)", iStr, warnI, badgeI);
    FF360LabsLookAndFeel::drawStatTile(g, rect2, "LUFS-S (SHORT)", sStr, false);
    FF360LabsLookAndFeel::drawStatTile(g, rect3, "LUFS-M (MOMENT)", mStr, false);
    FF360LabsLookAndFeel::drawStatTile(g, rect4, "LRA (RANGE)", lraStr, currentData.lra > 14.0f);
}

void LufsMeterModule::paintModule(juce::Graphics& g)
{
    auto bounds = getModuleBounds().toFloat().reduced(6.0f);
    
    // Top control strip gap
    bounds.removeFromTop(32.0f);

    float clusterW = juce::jlimit(140.0f, 180.0f, bounds.getWidth() * 0.38f);
    auto clusterArea = bounds.removeFromRight(clusterW);
    bounds.removeFromRight(8.0f); // Gap
    auto dialArea = bounds;

    drawLufsDial(g, dialArea);
    drawStatCluster(g, clusterArea);
}

void LufsMeterModule::resizedModule()
{
    auto bounds = getModuleBounds().reduced(6);
    auto topStrip = bounds.removeFromTop(26);
    
    targetSelector.setBounds(topStrip.removeFromLeft(210));
    resetButton.setBounds(topStrip.removeFromRight(65));
}
