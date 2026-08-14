# ff360_labs Modular Audio Metering Plugin — Phase 6: Visual Redesign

Prerequisite: Phase 1 Revision (`FF360LabsLookAndFeel` palette + shared helpers) is implemented first. This phase redraws each existing module's `paint()` to use the new glassmorphic gold identity and to feel more contemporary — moving away from flat bars/needles toward the kind of layered, gauge-driven, tile-based dashboard aesthetic common in professional metering plugins, fully reskinned to ff360_labs gold rather than any specific product's exact look.

Act as a senior C++ / JUCE UI engineer with a strong eye for modern audio-tool interface design. Implement the following module redesigns.

---

### 6.1 — Dashboard Shell & Module Chrome
1. Module headers: replace the plain header bar with a slim rounded-top header, module title left-aligned in the warm off-white, a thin gold underline instead of a full block color fill.
2. Give each module tile a consistent card treatment (glass fill, rounded corners, hairline border, top sheen) per the Phase 1 Revision spec — apply this once in `MeterModule`'s base `paint()` so every subclass inherits it automatically rather than each module drawing its own card background.
3. Add small status-tile styling for any module that shows a single key numeric readout (e.g. an integrated LUFS value, a true-peak value) — a compact rounded tile with the label small and top-aligned, the number large and centered, similar in spirit to a stat-card. Use this for `LufsMeterModule`'s target-validator readouts from Phase 5.2 in particular.

### 6.2 — Peak/RMS Meter Redesign
1. Replace flat rectangular bars with rounded-cap gradient bars (gold at low/mid level, transitioning to amber-red above the -3dB threshold), matching the gradient-fill logic already specified in Phase 4 but using the new palette.
2. Add a subtle glow on the illuminated portion of each bar (soft blur behind the fill, low opacity) rather than a hard-edged fill.
3. Add a thin horizontal threshold marker line at the -3dB point across both channel bars, gold at low opacity, so the danger zone is visually anchored rather than only implied by the gradient.

### 6.3 — VU Meter Redesign
This is the module you flagged as needing the most work.
1. Redesign the faceplate as a dark arc gauge (not a boxy rectangle background) — brushed charcoal arc background, gold tick marks, amber-red zone only in the last ~10% of the arc near/above 0 VU, rather than a wide redline zone.
2. Redraw the needle as a thin gold line with a soft glow trail and a subtle drop shadow at the pivot point, animated with a light spring/damping easing on movement (not a linear snap) for a more premium feel.
3. Add a small digital readout beneath the arc (current VU value, e.g. "-0.0") in a rounded stat-tile per 6.1's shared styling, rather than only relying on the analog needle.

### 6.4 — LUFS Meter Redesign
1. Redesign as a large dial/gauge (arc-style, similar language to the VU redesign in 6.3 but scaled for LUFS range) showing momentary/short-term/integrated as differently-weighted markers or a multi-pointer arc, with the primary integrated value as the large central digital readout.
2. Use the shared stat-tile styling (6.1.3) for the target-validator readouts (LUFS-I, LUFS-S, LUFS-M, True Peak) as a small tile cluster beside or below the main dial, each with a gold checkmark or amber-red flag depending on target compliance.
3. Keep the gating/measurement logic from Phase 5.2 untouched — this section is presentation-only.

### 6.5 — Phase Scope Redesign
1. Keep the Lissajous/goniometer plotting logic from Phase 5.3 untouched; update only the rendering: gold trace with the fading persistence effect using a gold-to-transparent gradient per point-age, rather than a flat single color.
2. Add a compact numeric correlation readout tile (per 6.1.3 styling) rather than a bare number floating in the module.

### 6.6 — Spectrum Analyzer & Histogram Redesign
1. Spectrum Analyzer: gradient-filled bars/line per-frequency using the gold-to-amber logic from 6.2, log-frequency axis labels in the warm off-white, subtle gridlines at low opacity rather than solid lines.
2. Histogram: rounded-cap horizontal bars, modal bucket highlighted in gold with the rest in a dimmer charcoal-gold tint, consistent card chrome per 6.1.

### 6.7 — Motion & Polish Pass
1. Audit all modules for one consistent easing/damping curve on animated values (meter bars, needle, dial pointers) — extract this into a shared utility if not already centralized, so "modern and stylish" motion feels consistent across the whole dashboard rather than module-by-module.
2. Verify the Phase 5.7 scalability work still holds: glow/blur effects and rounded corners should scale proportionally with the rest of the UI, not stay fixed-pixel.

---

Please start with **6.1 (Dashboard Shell & Module Chrome)** since every other module redesign depends on the shared card/tile styling it establishes, then move to **6.3 (VU Meter)** next since that's the module you're least happy with currently.
