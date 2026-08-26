#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class FF360LabsLookAndFeel : public juce::LookAndFeel_V4
{
public:
    struct Palette
    {
        static inline const juce::Colour backgroundDark    { 0xFF0A0A0B };
        static inline const juce::Colour containerDark     { 0xFF17171A };
        static inline const juce::Colour containerOutline  { 0xFF26262B };
        static inline const juce::Colour accentGold        { 0xFFC9A15A };
        static inline const juce::Colour accentAmberRed    { 0xFFE8654A };
        static inline const juce::Colour textOffWhite      { 0xFFEDEAE3 };
        static inline const juce::Colour textMuted         { 0xFF8A8780 };
        static inline const juce::Colour glassPanelFill    { 0x8017171A }; // ~50% alpha
        static inline const juce::Colour hairlineBorder    { 0x2EC9A15A }; // ~18% alpha gold
        static inline const juce::Colour goldGlow          { 0x59C9A15A }; // ~35% alpha gold
    };

    FF360LabsLookAndFeel();
    ~FF360LabsLookAndFeel() override = default;

    // Shared Static Drawing Helpers
    static void drawGlassPanel (juce::Graphics& g, juce::Rectangle<float> bounds, float cornerRadius = 8.0f);
    static juce::Colour getWarningColour();
    static juce::Colour getReadoutColour (bool isWarning);
    static bool isColorblindModeActive();
    static void setColorblindModeActive (bool active);
    static void drawStatTile (juce::Graphics& g, juce::Rectangle<float> bounds,
                              const juce::String& label, const juce::String& value,
                              bool isWarning = false, const juce::String& badgeText = "");

    // Font Helpers
    // getCustomFont / getNumericReadoutFont: monospace, for technical readouts, scale
    // ticks, and data labels (mirrors the mockup's pervasive use of JetBrains Mono).
    static juce::Font getCustomFont (float height = 14.0f, int styleFlags = juce::Font::plain);
    static juce::Font getNumericReadoutFont (float height = 18.0f);
    // getUiFont: proportional sans, reserved for interactive chrome — buttons, popup
    // menus, and module header titles — matching the mockup's Inter/JetBrains Mono
    // split where gold/mono is for data and sans is for navigation chrome.
    static juce::Font getUiFont (float height = 13.0f, int styleFlags = juce::Font::plain);

    // Component Styling Overrides
    void drawButtonBackground (juce::Graphics& g,
                               juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics& g,
                         juce::TextButton& button,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;

    void drawToggleButton (juce::Graphics& g,
                           juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;
                           
    void drawLabel (juce::Graphics& g, juce::Label& label) override;

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override;

    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override;
    
    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu,
                            const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;
};
