# ff360_labs Modular Audio Metering Plugin — Phase 5

Continuing the same project (JUCE 8.x, CMake, VST3/AU/Standalone, `ff360_labs` brand styling via `FF360LabsLookAndFeel`). Phases 0–4 are complete: the dashboard/module architecture, DSP FIFO pipeline, and a working `PeakRmsMeterModule` are in place, alongside placeholder `VU`, `LUFS`, and `PhaseScope` modules registered in the dashboard's "Add Module" menu.

Phase 5 replaces those placeholders with real, functioning meter modules, and adds two new module types (Spectrum Analyzer, Histogram) to round out the module catalog to match the Decibel-style scope established in Phase 4.

Act as a senior C++ audio software engineer and JUCE expert. Please implement the following step-by-step.

---

### **Phase 5: Extended Meter Module Catalog**

#### 5.1 — VU Meter Module (`VuMeterModule`)
1. Implement classic VU ballistics in `VuDSP.h`: 300ms integration time constant, calibrated so 0 VU ≈ -18 dBFS (adjustable reference level as a named constant).
2. Render an analog-style needle sweep in `paint()` using `FF360LabsLookAndFeel` colors — cyan needle/scale, orange redline zone past 0 VU, off-white tick labels.
3. Reuse the Phase 3 `juce::AbstractFifo` handoff pattern for audio-thread → GUI-thread data transfer; do not duplicate a second FIFO mechanism.

#### 5.2 — LUFS Meter Module (`LufsMeterModule`)
1. Implement momentary, short-term, and integrated LUFS measurement per **ITU-R BS.1770-4** in `LufsDSP.h`, including K-weighting filter and gating (relative + absolute).
2. Display all three values numerically plus a loudness range (LRA) readout.
3. Add a target-validator strip (e.g. -14 LUFS streaming target, -23 LUFS broadcast target) as selectable presets, styled with cyan (in-range) / orange (out-of-range) indicators consistent with the Peak/RMS threshold treatment from Phase 4.

#### 5.3 — Phase Scope Module (`PhaseScopeModule`)
1. Implement a Lissajous-style L/R correlation goniometer in `PhaseScopeDSP.h`, plotting sample pairs rotated 45° (mid/side axes).
2. Add a numerical phase correlation meter (-1 to +1) beneath or beside the scope, orange below 0 (phase risk), cyan above.
3. Use a fading trail/persistence effect (short decay per frame) rather than single-point plotting, for a readable trace at high sample rates — cap the persistence buffer size and document the tradeoff between trace smoothness and CPU cost.

#### 5.4 — Spectrum Analyzer Module (`SpectrumAnalyzerModule`) — new module type
1. Implement an FFT-based real-time spectrum analyzer in `SpectrumDSP.h` using `juce::dsp::FFT`, with a Hann window and configurable FFT order (default 2048).
2. Render as a log-frequency-scaled bar or line spectrum, gradient-filled cyan-to-orange by amplitude, matching the Peak/RMS gradient logic from Phase 4.
3. Smooth frame-to-frame using a simple peak-hold + decay so the display doesn't flicker at high frame rates.

#### 5.5 — Histogram Module (`HistogramModule`) — new module type
1. Implement a rolling loudness histogram in `HistogramDSP.h`, bucketing incoming LUFS or RMS readings (reuse 5.2's LUFS engine as the data source) over a configurable time window (default 5 minutes).
2. Render as a horizontal bar histogram, styled consistently with the rest of the dashboard, with the modal (most common) loudness bucket highlighted in cyan.
3. Include a reset control in the module header.

#### 5.6 — Dashboard Integration
1. Register all five module types in the "Add Module" menu built in Phase 4, replacing the lightweight placeholders with these full implementations.
2. Verify each module independently supports Grid Mode resizing and Focus Mode maximize/restore, using the existing `MeterDashboard` logic — no changes to the dashboard/layout manager should be required if Phase 2's `MeterModule` contract was followed correctly. If changes ARE required, treat that as a signal the base class contract needs revisiting, not a one-off patch.
3. Confirm CPU usage stays reasonable with all five modules active simultaneously in Grid Mode; note any module that needs frame-rate throttling (e.g. Spectrum Analyzer redraw rate) independent of the others.

#### 5.7 — Scalable Interface
This touches `PluginEditor` and `MeterDashboard` from earlier phases, not just the new Phase 5 modules — treat it as a cross-cutting pass over the whole UI, not a new module.

1. Make the plugin window user-resizable via `setResizable()` with sensible min/max bounds, and persist the last-used window size in plugin state so it's recalled on reopen.
2. Apply a single global UI scale factor (driven by `juce::Component::setTransform()` or JUCE's built-in `AudioProcessorEditor` scaling, whichever fits the CMake/JUCE 8 setup from Phase 0) so all modules, fonts, and the nav bar scale together rather than independently — avoid per-module scale logic.
3. Ensure `FF360LabsLookAndFeel` uses relative/proportional sizing (percentages of component bounds, scalable font heights) rather than hardcoded pixel values, so text and vector-drawn meters stay crisp at both small and large window sizes.
4. Verify `MeterDashboard`'s `juce::Grid` reflows module columns/rows sensibly at both minimum and maximum window size — confirm a minimum readable size per module type (e.g. Spectrum Analyzer and Phase Scope need more room than a numeric LUFS readout) and enforce that as a per-module minimum size hint the Grid respects.
5. Confirm behavior on HiDPI/Retina displays and mixed-DPI multi-monitor setups (JUCE's native DPI scaling should mostly handle this — verify rather than reimplement).

---

Please guide me through **5.1 (VU Meter)** first, since it reuses the most existing infrastructure. Provide `VuDSP.h` and `VuMeterModule` together, then show the dashboard registration change. We'll cover 5.7 (scalability) as a pass once the module catalog is complete.
