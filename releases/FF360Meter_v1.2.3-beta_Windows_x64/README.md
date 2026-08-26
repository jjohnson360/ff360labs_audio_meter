# ff360_labs Modular Audio Meter

**Beta v1.2.3** — A professional, modular audio metering and mastering analysis suite built with C++20 and JUCE 8.

> **Beta Notice**: This is a pre-release beta build. Core metering accuracy has been audited and verified against reference sine tones through Phase 10. Please report any measurement discrepancies or platform-specific issues.

See [CHANGELOG.md](./CHANGELOG.md) for the full version history, including the Beta v1.2.3 Phase Scope rendering fix and UI style pass, the Beta v1.2.1 Phase Scope hotfix, and the Beta v1.2.0 calibration audit.

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
- **Phase Scope**: Goniometer radar with Lissajous decaying persistence traces (0.75 alpha for snappier feel), polar graticule, and correlation stat-tiles. *(Phase 10: WASAPI freeze bug fixed; Phase 10.4 hotfix in v1.2.1 corrected a display regression introduced by that fix; v1.2.3 made the persistence decay time-based instead of paint-call-based, fixing a smeared/overly-persistent trail on Windows.)*

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

## Installation & Deployment Guide

### Windows 10/11 (x64) — Beta v1.2.3

> **Beta:** This release is pre-release software. Verify calibration via the `CAL TEST` button in the VU module (debug build) or against a known reference signal before use in production sessions.

1. Extract `FF360Meter_v1.2.3-beta_Windows_x64.zip`.
2. Copy `FF360Meter.vst3` to your system VST3 directory:
   ```text
   C:\Program Files\Common Files\VST3\
   ```
3. To run standalone, launch `FF360Meter.exe`.
4. For system audio monitoring: open **⚙ Settings → Audio I/O Settings** and select a Stereo Mix / WASAPI loopback / VB-Cable input device.

### macOS 12 and Later (Apple Silicon & Intel) — Beta v1.2.3

> **Beta:** Universal Binary (arm64 + x86_64). Notarization not yet applied for this beta build. macOS may require manual security approval on first launch: **System Settings → Privacy & Security → Open Anyway**.

1. Extract `FF360Meter_v1.2.3-beta_macOS_Universal.zip`.
2. Copy `FF360Meter.vst3` to:
   ```text
   /Library/Audio/Plug-Ins/VST3/
   ```
3. Copy `FF360Meter.component` (Audio Unit) to:
   ```text
   /Library/Audio/Plug-Ins/Components/
   ```
4. Copy `FF360Meter.app` to `/Applications/`.
5. For system audio monitoring: install [BlackHole 2ch](https://github.com/ExistentialAudio/BlackHole), create a Multi-Output Device in Audio MIDI Setup, and select BlackHole as the Input Device in **⚙ Settings → Audio I/O Settings**.

---

## Known Issues & Beta Limitations

| # | Issue | Status |
|---|---|---|
| B-01 | macOS notarization not applied — Gatekeeper requires manual approval on first launch | Open |
| B-02 | UI Size scaling uses `setSize()` — does not scale internal font or HiDPI contexts; proper DPI-aware scaling deferred to v1.3 | Open |
| B-03 | `CAL TEST` button only available in Debug builds — no release-mode calibration report | By design |
| B-04 | Phase Scope persistence trail speed tied to timer rate — may differ slightly at non-60Hz display rates | Open |

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

## Release Notes

Full details for every release, including bug fixes, new features, and polish items, now live in [CHANGELOG.md](./CHANGELOG.md).