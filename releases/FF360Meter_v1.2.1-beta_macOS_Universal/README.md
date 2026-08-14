# ff360_labs Modular Audio Meter

**Beta v1.2.0** — A professional, modular audio metering and mastering analysis suite built with C++20 and JUCE 8.

> **Beta Notice**: This is a pre-release beta build. Core metering accuracy has been audited and verified against reference sine tones through Phase 10. Please report any measurement discrepancies or platform-specific issues.

---

## Overview
**ff360_labs Modular Audio Meter** is an extensible, multi-module audio analysis plugin designed for mixing, mastering, and broadcast compliance workflows. Built on a glassmorphic dark-mode design system with metallic gold accents, it features a dynamic grid dashboard, hardware-styled segmented LED meters, analog-modeled VU ballistics, and exportable session reports.

---

## Key Features & Metering Modules

### 1. Dynamic Modular Dashboard
- **Grid & Focus Layouts**: Auto-flowing responsive grid layouts and single-module focus mode.
- **Factory Layout Presets**: Built-in suites (*Mastering*, *Broadcast QC*, *Quick Check*, *Full Suite*) with custom layout saving and APVTS session recall.
- **Detachable Floating Windows**: Pop out any module into an always-on-top, resizable floating window with seamless re-docking on close.
- **Consolidated Settings Menu (⚙)**: Audio I/O, Export, Accessibility, Grid/Focus mode, UI Size (50%–200%), Full Screen, and About — all accessible from a single top-left dropdown. Nav bar reduces to: branding | ⚙ | Add Module | Layout preset | two status dots.
- **Per-Module Options Menu (⋮)**: Each module header shows a single kebab dropdown with Resize, Detach to Window, and Close/Remove entries, consistent across all module types.

### 2. Metering Suite
- **Peak / RMS Meter**: Hardware-inspired discrete segmented LED level bars (~48 segments) with illuminated topmost glow, core RMS luminance, and horizontal −3 dB threshold markers. *(Phase 10: RMS accuracy fixed — double-sqrt bug resolved.)*
- **VU Meter**: Analog-modeled arc gauge with true windowed RMS power integration (x² → IIR → √), 1:1 needle tracking (0ms secondary lag), adjustable calibration reference levels (−18, −20, −14, −12, −10 dBFS), and real-time Dev Timing Oscilloscope (`DEV OSC`). *(Phase 10: AES-17 ×2 bias removed; built-in calibration test suite.)*
- **LUFS Meter (ITU-R BS.1770-4)**: Multi-pointer circular dial displaying Integrated, Short-Term, and Momentary loudness with target compliance stat-tiles (`PASS`, `HIGH`, `LOW`).
- **Target Profile Presets**: Built-in platform targets for Spotify (−14 LUFS), YouTube (−14 LUFS), Apple Music (−16 LUFS), Netflix (−27 LUFS), EBU R128 (−23 LUFS), Club/Master (−9 LUFS), AES Streaming (−16 LUFS), and Custom targets.
- **Spectrum Analyzer**: Dual L/R channel overlaid traces (gold + desaturated gold-gray), 2048-point log-scaled FFT (20Hz–20kHz) with peak-hold ballistics, 3-bin moving-average smoothing, and a user-selectable FFT resolution (1024 / 2048 / 4096). *(Phase 10: dual-channel, smoother decay, resolution selector added.)*
- **Histogram (5-Min Rolling & A/B Compare)**: Real-time loudness distribution histogram with modal peak highlights, snapshot capture (`CAP A`), and dual-pass A/B compare overlay with real-time modal delta readouts.
- **Phase Scope**: Goniometer radar with Lissajous decaying persistence traces (0.75 alpha for snappier feel), polar graticule, and correlation stat-tiles. *(Phase 10: WASAPI freeze bug fixed — unbounded vector accumulation resolved.)*

### 3. Workflow & Standalone Audio Capture
- **Standalone System Audio (Loopback) Capture**: In-app Audio I/O settings (accessible via ⚙ menu), real-time input status indicators (`●` dot: cyan=live, amber=idle, red=no input), and native OS loopback routing guides (Windows WASAPI/Stereo Mix, macOS BlackHole/Loopback, Linux PipeWire).
- **Session Report Export**: One-click export of session metrics (LUFS-I, LRA, Short-Term Max, Peak L/R, True Peak) as CSV or branded HTML mastering deliverables.
- **Adaptive Frame Rate & Performance Budgeting**: Per-module target refresh rates with automated load monitoring (displayed as a `●` colour dot in the nav bar).
- **Colorblind-Safe Accessibility Palette**: Toggle in ⚙ Settings menu swapping warning tones for high-contrast Accessible Sky Blue (`#38bdf8`).

---

## Architecture
- **Lock-Free Concurrency**: Audio DSP algorithms run entirely lock-free on the realtime audio thread, publishing frames to GUI visualizers via circular `AudioFifo` buffers.
- **Single Source of Timing Truth**: DSP performs exact mathematical ballistics integration while visualizers render 1:1 directly, preventing stacked smoothing latency.
- **Centralized LookAndFeel**: The entire interface is driven by `FF360LabsLookAndFeel` with custom typography, frosted glass panels, hairline metallic gold borders, and stat-tile components.
- **Full State Serialization**: Plugin layouts, calibration levels, active target profiles, and accessibility preferences persist via `juce::AudioProcessorValueTreeState` and `juce::ValueTree`.
- **Built-in Calibration Suite** (`CalibrationTest.h`): Static `CalibrationTestRunner` generates reference sine tones at known dBFS levels and verifies each DSP module reports the mathematically expected value (tolerance ±0.25 dB). Active in debug builds via `CAL TEST` button on the VU meter module.

---

## Build Instructions
```bash
# Regenerate build files (required after CMakeLists changes)
cmake -B build -S .

# Build (VST3 and Standalone)
cmake --build build --config Release
```

### System Requirements
- JUCE 8.x
- CMake 3.22+
- C++20 compliant compiler (MSVC 2022, Clang 14+, or GCC 12+)
- Windows 10/11 (x64) or macOS 12+ (Universal Binary: Apple Silicon / Intel)

---

## Release Notes — Beta v1.2.0 (August 14, 2026)

### Hotfix — Beta v1.2.1 (August 14, 2026)
- **[REGRESSION] Phase Scope blank / no display**: The Phase 10.4 WASAPI freeze fix incorrectly placed `samplePairs.clear()` immediately after `repaint()` in `timerCallback()`. Because `repaint()` is asynchronous, the vector was already empty by the time `updateScopeImage()` ran. Fixed: clear moved back inside `updateScopeImage()` after drawing; zero-bounds early-return also clears explicitly; 8192-pair cap in `timerCallback()` remains as runaway protection.

### Bug Fixes — Beta v1.2.0 (August 14, 2026)
- **[CRITICAL] RMS accuracy — double-sqrt bug** (`PeakRmsDSP`): `buffer.getRMSLevel()` already returns RMS; the extra `std::sqrt()` wrapper was computing `RMS^0.5`, making all RMS readings run consistently hot. Resolved.
- **[CRITICAL] VU calibration — AES-17 ×2 bias** (`VuDSP`): A `* 2.0f` multiplier added a constant phantom +3 dB to all VU readings. Removed; now uses plain `sqrt(power)` = true RMS.
- **[PLATFORM] Phase Scope freeze on Windows / WASAPI**: `samplePairs.clear()` was inside `updateScopeImage()` which bails early on zero-size bounds — causing the accumulation vector to grow unbounded on WASAPI where callbacks arrive before `resized()` propagates. Fixed: unconditional clear in `timerCallback()` + 8192-pair hard cap.

### New Features
- **⚙ Settings Menu**: Consolidates Audio I/O, Export, Accessibility, Grid/Focus, UI Size, Full Screen (standalone), and About into a single dropdown. Nav bar now uncluttered.
- **⋮ Per-Module Kebab Menu**: Replaces three separate header buttons (X / [] / [^]) with a single options menu per module, inherited by all module types via `MeterModule` base class.
- **Spectrum Dual L/R Traces**: Left channel renders in full gold with gradient fill; Right channel renders in desaturated gold-gray behind it. Both channels visible simultaneously.
- **Spectrum FFT Resolution Selector**: Low (1024) / Medium (2048) / High (4096) — with tooltip documenting the CPU/latency tradeoff.
- **VU Arc Density**: Five new graduated tick marks added (−15, −12, −8, −6, −4 VU) for easier at-a-glance position reading.
- **Built-in Calibration Test Suite**: `CalibrationTest.h` documents expected readings for all meter types; `CAL TEST` button in debug builds runs the full suite and shows PASS/FAIL results in the Dev Oscilloscope overlay.
- **Beta Version Labeling**: About dialog now shows `Beta v1.2.0 (Aug 14 2026)`.
- **WASAPI Loopback**: Confirmed available — `inspectPlatformLoopback()` scans for Stereo Mix, virtual cable, and loopback devices. Access via ⚙ → Audio I/O Settings.

### Polish
- Peak/RMS unlit segment opacity halved (0.06 → 0.03) for cleaner active-segment contrast.
- Phase Scope persistence trail reduced (0.82 → 0.75 alpha) for snappier feel.
- Spectrum decay smoothing time constant increased (0.1s → 0.18s) + 3-bin moving average for smoother curves.

