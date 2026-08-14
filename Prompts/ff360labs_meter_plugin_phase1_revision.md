# ff360_labs Modular Audio Metering Plugin — Phase 1 Revision (Visual Identity)

This supersedes the color/styling spec from the original Phase 1 section. Architecture, class names, and build setup from Phases 0–5 are unchanged — this is a palette and styling pass on `FF360LabsLookAndFeel` only.

**Reason for the change:** the original spec used a generic cyan/orange "technical dashboard" palette. It didn't match the established ff360_labs brand (glassmorphic dark UI, deep black, metallic gold), so the plugin read as off-brand. This revision brings it in line with the actual brand identity.

For structural/layout inspiration (module tiling, gauge-style readouts, stat tiles), this revision draws loosely on the general conventions of dashboard-style metering plugins in the market — reskin fully to the palette below rather than copying any specific product's exact visual treatment.

---

### Updated Color Spec

Act as a senior C++ / JUCE UI engineer. Update `FF360LabsLookAndFeel` to use the following palette, replacing all prior color constants:

| Role | Old Value | New Value | Notes |
|---|---|---|---|
| Background Dark | `#0D0F12` | `#0a0a0b` | Matches ff360_labs site "Deep Black" |
| Container Dark | `#16191E` | `#17171a` | Matches ff360_labs site "Matte Charcoal" |
| Primary Accent (active signal) | Cyber Cyan `#00E5FF` | Metallic Gold `#c9a15a` | Primary highlight, active meter fill, selected states |
| Secondary Accent (peak/warning) | Signal Orange `#FF6B00` | Warm Amber-Red `#e8654a` | Stays in the "warm metal" family instead of clashing with gold |
| Text / Indicators | `#E0E6ED` | `#EDEAE3` | Slight warm tint so it sits naturally against gold, rather than a cool off-white |
| Glass panel fill (new) | — | `#17171a` @ 40–60% opacity | Used for module backgrounds over the base background, simulating frosted glass |
| Hairline border (new) | — | `#c9a15a` @ 15–20% opacity | Thin light-catching edge on module panels, replaces hard charcoal borders |

Keep all values as named `constexpr` or `juce::Colour` constants in one place (a `Colours` or `Palette` namespace within `FF360LabsLookAndFeel`) so nothing is hardcoded inline in individual modules — this matters because Phase 6 will touch every module's `paint()` and needs a single source of truth to update from.

### Component Styling Updates

1. **Module panels:** rounded corners (6–10px radius), glass panel fill per above, hairline gold border. Add a subtle top-edge gradient (slightly lighter opacity at the top few pixels) to fake a glass "sheen."
2. **Buttons/toggles:** replace flat-fill active states with a soft gold glow (a low-opacity gold `DropShadow` or blurred rect behind the component) rather than a hard color swap.
3. **Labels/readouts:** numeric readouts (dB values, LUFS numbers) should render in a slightly larger weight than section labels, gold-tinted when the reading is in a "good" range, amber-red when in a warning/peak range — this pattern should be defined once as a shared helper (e.g. `FF360LabsLookAndFeel::getReadoutColour(bool isWarning)`) so Phase 5 modules can reuse it instead of each reimplementing the logic.
4. **Focus/Grid toggle buttons:** give the active mode a gold underline or pill-shaped highlight rather than a filled background swap, for a lighter, more "modern dashboard" feel.

---

Please implement the updated `FF360LabsLookAndFeel` constants and helper methods first, then confirm which existing modules (from Phases 2–5) need their `paint()` calls touched to pick up the new constants automatically vs. which have hardcoded old colors that need manual replacement.
