#include "FF360LabsLookAndFeel.h"
#include "../../Core/Constants.h"

FF360LabsLookAndFeel::FF360LabsLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, Palette::backgroundDark);
    setColour(juce::TextButton::buttonColourId, Palette::containerDark);
    setColour(juce::TextButton::textColourOffId, Palette::textOffWhite);
    setColour(juce::TextButton::textColourOnId, Palette::accentGold);
    
    setColour(juce::Label::textColourId, Palette::textOffWhite);
    setColour(juce::ComboBox::backgroundColourId, Palette::containerDark);
    setColour(juce::ComboBox::textColourId, Palette::textOffWhite);
    setColour(juce::ComboBox::outlineColourId, Palette::hairlineBorder);
    setColour(juce::ComboBox::arrowColourId, Palette::accentGold);

    setColour(juce::PopupMenu::backgroundColourId, Palette::containerDark);
    setColour(juce::PopupMenu::textColourId, Palette::textOffWhite);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, Palette::accentGold.withAlpha(0.20f));
    setColour(juce::PopupMenu::highlightedTextColourId, Palette::textOffWhite);
}

juce::Font FF360LabsLookAndFeel::getCustomFont(float height, int styleFlags)
{
    // Was hardcoded to "Consolas" (Windows-only, silently falls back on macOS).
    // juce::Font::getDefaultMonospacedFontName() resolves per-platform (Consolas on
    // Windows, Menlo on macOS) while keeping the same technical/mono identity used
    // throughout every module's data labels and scale ticks.
    return juce::FontOptions(height, styleFlags).withName(juce::Font::getDefaultMonospacedFontName());
}

juce::Font FF360LabsLookAndFeel::getNumericReadoutFont(float height)
{
    return juce::FontOptions(height, juce::Font::bold).withName(juce::Font::getDefaultMonospacedFontName());
}

juce::Font FF360LabsLookAndFeel::getUiFont(float height, int styleFlags)
{
    return juce::FontOptions(height, styleFlags).withName(juce::Font::getDefaultSansSerifFontName());
}

void FF360LabsLookAndFeel::drawGlassPanel (juce::Graphics& g, juce::Rectangle<float> bounds, float cornerRadius)
{
    // Base Frosted Glass Fill
    g.setColour(Palette::glassPanelFill);
    g.fillRoundedRectangle(bounds, cornerRadius);

    // Subtle Top-edge Sheen Gradient (lighter opacity at the top few pixels)
    juce::ColourGradient sheen(juce::Colours::white.withAlpha(0.06f),
                               bounds.getX(), bounds.getY(),
                               juce::Colours::transparentWhite,
                               bounds.getX(), bounds.getY() + 18.0f,
                               false);
    g.setGradientFill(sheen);
    g.fillRoundedRectangle(bounds, cornerRadius);

    // Hairline Metallic Gold Border
    g.setColour(Palette::hairlineBorder);
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerRadius, 1.0f);
}

static bool colorblindModeEnabled = false;

bool FF360LabsLookAndFeel::isColorblindModeActive()
{
    return colorblindModeEnabled;
}

void FF360LabsLookAndFeel::setColorblindModeActive (bool active)
{
    colorblindModeEnabled = active;
}

juce::Colour FF360LabsLookAndFeel::getWarningColour()
{
    return colorblindModeEnabled ? juce::Colour (0xFF38BDF8) : Palette::accentAmberRed;
}

juce::Colour FF360LabsLookAndFeel::getReadoutColour (bool isWarning)
{
    return isWarning ? getWarningColour() : Palette::accentGold;
}

void FF360LabsLookAndFeel::drawStatTile (juce::Graphics& g, juce::Rectangle<float> bounds,
                                         const juce::String& label, const juce::String& value,
                                         bool isWarning, const juce::String& badgeText)
{
    // Draw Glass Tile Chrome
    drawGlassPanel(g, bounds, 6.0f);

    auto inner = bounds.reduced(5.0f, 3.0f);

    // Stat Label (Small, top-aligned, muted warm)
    auto topArea = inner.removeFromTop(13.0f);
    
    if (badgeText.isNotEmpty())
    {
        auto badgeArea = topArea.removeFromRight(38.0f);
        juce::Colour badgeCol = isWarning ? getWarningColour() : Palette::accentGold;
        
        g.setColour(badgeCol.withAlpha(0.18f));
        g.fillRoundedRectangle(badgeArea.reduced(1.0f, 1.0f), 3.0f);
        g.setColour(badgeCol);
        g.drawRoundedRectangle(badgeArea.reduced(1.0f, 1.0f), 3.0f, 0.9f);
        
        g.setFont(getCustomFont(8.0f, juce::Font::bold));
        g.drawText(badgeText, badgeArea, juce::Justification::centred, false);
        
        topArea.removeFromRight(4.0f);
    }

    g.setFont(getCustomFont(9.0f, juce::Font::bold));
    g.setColour(Palette::textMuted);
    g.drawText(label, topArea, juce::Justification::centredLeft, false);

    // Numeric Readout (Bold monospace)
    g.setFont(getNumericReadoutFont(14.0f));
    g.setColour(getReadoutColour(isWarning));
    g.drawText(value, inner, juce::Justification::centredRight, false);
}

void FF360LabsLookAndFeel::drawButtonBackground (juce::Graphics& g,
                                                 juce::Button& button,
                                                 const juce::Colour& /*backgroundColour*/,
                                                 bool shouldDrawButtonAsHighlighted,
                                                 bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto cornerSize = 4.0f;
    bool isToggled = button.getToggleState();

    if (shouldDrawButtonAsDown || isToggled)
    {
        // Soft gold glow behind/around button
        g.setColour(Palette::goldGlow);
        g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 1.5f);

        // Glass background with subtle gold tint
        g.setColour(Palette::containerDark.withAlpha(0.85f));
        g.fillRoundedRectangle(bounds, cornerSize);

        // Gold pill / underline highlight for active mode
        auto underlineArea = bounds.removeFromBottom(2.0f).reduced(4.0f, 0.0f);
        g.setColour(Palette::accentGold);
        g.fillRoundedRectangle(underlineArea, 1.0f);
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        g.setColour(Palette::containerDark.brighter(0.05f).withAlpha(0.8f));
        g.fillRoundedRectangle(bounds, cornerSize);

        g.setColour(Palette::accentGold.withAlpha(0.40f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 1.0f);
    }
    else
    {
        g.setColour(Palette::glassPanelFill);
        g.fillRoundedRectangle(bounds, cornerSize);

        g.setColour(Palette::hairlineBorder);
        g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 1.0f);
    }
}

void FF360LabsLookAndFeel::drawButtonText (juce::Graphics& g,
                                           juce::TextButton& button,
                                           bool /*shouldDrawButtonAsHighlighted*/,
                                           bool shouldDrawButtonAsDown)
{
    juce::Font font = getUiFont(12.5f, juce::Font::plain);
    g.setFont(font);

    bool isActive = shouldDrawButtonAsDown || button.getToggleState();
    juce::Colour textColour = isActive ? Palette::accentGold : Palette::textOffWhite;
    g.setColour(textColour);

    auto textBounds = button.getLocalBounds();
    g.drawText(button.getButtonText(), textBounds, juce::Justification::centred, true);
}

void FF360LabsLookAndFeel::drawToggleButton (juce::Graphics& g,
                                             juce::ToggleButton& button,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool /*shouldDrawButtonAsDown*/)
{
    auto bounds = button.getLocalBounds().toFloat();
    
    float size = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    juce::Rectangle<float> tickBounds (bounds.getX(), bounds.getCentreY() - size * 0.5f, size, size);

    g.setColour(shouldDrawButtonAsHighlighted ? Palette::containerOutline.brighter(0.1f) : Palette::containerOutline);
    g.fillRoundedRectangle(tickBounds, 2.0f);
    g.setColour(Palette::hairlineBorder);
    g.drawRoundedRectangle(tickBounds.reduced(0.5f), 2.0f, 1.0f);

    if (button.getToggleState())
    {
        g.setColour(Palette::accentGold);
        g.fillRoundedRectangle(tickBounds.reduced(2.5f), 1.0f);

        // Soft glow around active tick
        g.setColour(Palette::goldGlow);
        g.drawRoundedRectangle(tickBounds.reduced(0.5f), 2.0f, 1.5f);
    }

    g.setColour(button.getToggleState() ? Palette::textOffWhite : Palette::textMuted);
    g.setFont(getUiFont(12.5f));
    g.drawText(button.getButtonText(),
               bounds.withTrimmedLeft(tickBounds.getRight() + 6.0f).toNearestInt(),
               juce::Justification::centredLeft, true);
}

void FF360LabsLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.fillAll(label.findColour (juce::Label::backgroundColourId));

    if (! label.isBeingEdited())
    {
        auto alpha = label.isEnabled() ? 1.0f : 0.5f;
        const juce::Font font = getCustomFont(13.0f);

        g.setColour (label.findColour (juce::Label::textColourId).withMultipliedAlpha (alpha));
        g.setFont (font);

        auto textArea = label.getBorderSize().subtractedFrom (label.getLocalBounds());

        g.drawFittedText (label.getText(), textArea, label.getJustificationType(),
                          juce::jmax (1, (int) ((float) textArea.getHeight() / font.getHeight())),
                          label.getMinimumHorizontalScale());
    }
}

void FF360LabsLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                         int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                         juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float>(0.0f, 0.0f, (float)width, (float)height);
    drawGlassPanel(g, bounds, 4.0f);

    // Arrow indicator
    auto arrowZone = bounds.removeFromRight(20.0f).reduced(4.0f);
    juce::Path path;
    path.startNewSubPath(arrowZone.getX() + 2.0f, arrowZone.getCentreY() - 2.0f);
    path.lineTo(arrowZone.getCentreX(), arrowZone.getCentreY() + 3.0f);
    path.lineTo(arrowZone.getRight() - 2.0f, arrowZone.getCentreY() - 2.0f);

    g.setColour(box.isEnabled() ? Palette::accentGold : Palette::textMuted);
    g.strokePath(path, juce::PathStrokeType(1.5f));
}

void FF360LabsLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    auto bounds = juce::Rectangle<float>(0.0f, 0.0f, (float)width, (float)height);
    g.setColour(Palette::containerDark.withAlpha(0.95f));
    g.fillRoundedRectangle(bounds, 6.0f);

    g.setColour(Palette::hairlineBorder);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);
}

void FF360LabsLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                                              bool isSeparator, bool isActive, bool isHighlighted,
                                              bool /*isTicked*/, bool /*hasSubMenu*/,
                                              const juce::String& text, const juce::String& /*shortcutKeyText*/,
                                              const juce::Drawable* /*icon*/, const juce::Colour* /*textColour*/)
{
    if (isSeparator)
    {
        auto r = area.reduced(5, 0);
        g.setColour(Palette::hairlineBorder);
        g.fillRect(r.removeFromTop(1));
        return;
    }

    if (isHighlighted && isActive)
    {
        g.setColour(Palette::accentGold.withAlpha(0.18f));
        g.fillRoundedRectangle(area.toFloat().reduced(2.0f, 1.0f), 3.0f);
        
        g.setColour(Palette::accentGold);
    }
    else
    {
        g.setColour(isActive ? Palette::textOffWhite : Palette::textMuted);
    }

    g.setFont(getUiFont(12.5f));
    g.drawText(text, area.reduced(10, 0), juce::Justification::centredLeft, true);
}

