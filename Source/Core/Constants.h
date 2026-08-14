#pragma once
#include <juce_graphics/juce_graphics.h>

namespace ff360_labs
{
    // Signature ff360_labs Color Palette (Phase 1 Revision)
    const juce::Colour BackgroundDark    { 0xFF0A0A0B }; // Deep Black
    const juce::Colour ContainerDark     { 0xFF17171A }; // Matte Charcoal
    const juce::Colour ContainerOutline  { 0xFF26262B }; // Subtle container border
    const juce::Colour AccentGold        { 0xFFC9A15A }; // Metallic Gold (primary highlight / active)
    const juce::Colour AccentAmberRed    { 0xFFE8654A }; // Warm Amber-Red (peak / warning)
    const juce::Colour AccessibleSkyBlue { 0xFF38BDF8 }; // Accessible High-Contrast Sky Blue (Colorblind mode)
    const juce::Colour TextOffWhite      { 0xFFEDEAE3 }; // Warm off-white
    const juce::Colour TextMuted         { 0xFF8A8780 }; // Warm muted text

    // Glassmorphic styling constants
    const juce::Colour GlassPanelFill    { ContainerDark.withAlpha(0.50f) }; // 50% opacity frosted glass fill
    const juce::Colour HairlineBorder    { AccentGold.withAlpha(0.18f) };    // 18% opacity gold edge
    const juce::Colour GoldGlow          { AccentGold.withAlpha(0.35f) };    // Soft gold glow

    // Compatibility aliases for seamless transition from previous phases
    const juce::Colour AccentCyberCyan   = AccentGold;
    const juce::Colour AccentSignalOrange= AccentAmberRed;

    // Metering Constants
    constexpr float METER_FLOOR_DB   = -60.0f;
    constexpr float METER_CEILING_DB = 0.0f;
    constexpr float METER_WARNING_DB = -3.0f;
    constexpr double DEFAULT_SAMPLE_RATE = 48000.0;
}
