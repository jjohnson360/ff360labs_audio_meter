# ff360_labs Modular Audio Meter — Progress & Release Report

**Project**: `ff360labs_audio_meter`  
**Version**: `Beta v1.2.3`  
**Release Date & Time**: `August 26, 2026`  
**Author / Organization**: `ff360 Labs` (`jjohnson360`)  
**Target Platforms**: Windows 10/11 (x64) & macOS 12+ (Universal Binary: Apple Silicon / Intel)  
**Build Status**: ✅ Clean — MSVC 2022 Release, exit code 0, zero errors

---

## Executive Summary

The **ff360_labs Modular Audio Meter** is an extensible, hardware-styled audio analysis and mastering suite built with **C++20** and **JUCE 8.0.4**. Beta v1.2.3 delivers **Phase 12** — a fix for a Phase Scope rendering issue reported on Windows, and a UI style pass that tightens the app's typography, chrome coloring, and dashboard spacing to better match the project's reference dashboard mockup.

---

## Phase-by-Phase Development Log

### Phase 12: Windows Rendering Fix & Mockup Style Alignment ← **CURRENT**

#### 12.1 — Phase Scope Persistence Decay Fix (Windows)
- **Symptom**: on Windows, the Phase Scope's Lissajous persistence trail appeared smeared/overly persistent instead of the intended "snappier" fading trail.
- **Root cause**: `PhaseScopeModule::updateScopeImage()` faded the trail image by a fixed `×0.75` multiplier once per *paint call*, which assumed a steady ~60Hz cadence. `repaint()` only requests a paint — the OS decides when it's actually serviced — and this project enables no Direct2D or OpenGL rendering context, so Windows uses JUCE's default unaccelerated software/GDI renderer, which services paints far less consistently under load than macOS's compositor. A slower/irregular paint cadence meant the trail decayed once per a longer real-world interval than intended, fading far more slowly than designed.
- **Fix**: persistence decay is now computed from actual elapsed wall-clock time (new `lastDecayTimeMs` tracking in `PhaseScopeModule`), so the fade rate stays constant in real time regardless of how often the OS actually paints.

#### 12.2 — UI Style Pass Toward the Reference Mockup
- **Typography hierarchy**: added a dedicated sans-serif `FF360LabsLookAndFeel::getUiFont()` for interactive chrome (buttons, popup menus, toggle text, module header titles), separate from the monospace `getCustomFont()` / `getNumericReadoutFont()` reserved for technical data and numeric readouts — mirroring the reference mockup's Inter/JetBrains Mono split. Previously every element rendered in one hardcoded font ("Consolas"), which also silently failed to resolve on macOS.
- **Muted chrome**: module header titles and the app-bar subtitle moved from bright bold white to a quieter muted tone, reserving brightness/gold for live data and the brand name rather than structural chrome.
- **Dashboard grid gap** tightened from 4px to 2px for a denser panel layout closer to the mockup.
- Segmented LED meters, glassmorphic panel treatment, and gold hairline module borders were intentionally left unchanged — these are established product identity, not mockup deviations.

---

### Phase 11: Silence Diagnosis, Icon Fix & M4L Cross-Application

#### 11.1 — Real-Time DEV OSC Reference Generator & Silence Diagnosis
- **Internal 1 kHz Sine Tone**: Synthesized in `FF360MeterProcessor::processBlock` at -18 dBFS (0.12589 amp) when enabled, driving all meters to calibrated target levels.
- **Top Nav Bar DEV OSC Toggle**: Gold-illuminated toggle button providing immediate, self-contained verification of the entire DSP and FIFO rendering pipeline without external audio files or DAW tracks.
- **Audio Input Device Selector Dropdown**: Top bar dropdown in Standalone mode allowing instant switching and live re-initialization of audio endpoints and loopback devices (`AudioDeviceManager`).
- **4-State I/O Status Badge**: Distinguishes between DEV OSC Active (🟣), No Hardware Input Device (🔴), Input Connected & Silent (🟡), and Live Audio Streaming (🟢) with live peak dB display.

#### 11.2 — Module Header Vector Icon Encoding Fix
- **Vector-Rendered Kebab Button**: Replaced string-literal text buttons with native vector-rendered `KebabMenuButton` (`fillEllipse`), eliminating multi-byte encoding corruption (`â‹®`) and font glyph mismatch issues.
- **MSVC `/utf-8` Support**: Added `/utf-8` compiler flags in `CMakeLists.txt` to enforce UTF-8 execution character sets.

#### 11.3–11.6 — Max for Live Companion Specification
- Created `M4L_Accurate_Metering_Guidelines.md` detailing built-in `[cycle~]` reference tone routing, dynamic `[adstatus sr]` sample-rate scaling for 44.1/48/96 kHz, BS.1770-4 dual-gate LRA rules (-10 LU relative), and visual silence detection.

---

### Phase 0 & 1: Core Architecture & Brand Identity
- **Lock-Free Concurrency**: Implemented SPSC circular `AudioFifo` channels isolating realtime DSP threads from 60 FPS GUI rendering.
- **Brand Palette & Design System**: Created `FF360LabsLookAndFeel` — Deep Black (`#0a0a0b`), Matte Charcoal (`#17171a`), Metallic Gold (`#c9a15a`), Warm Amber-Red (`#e8654a`), Accessible Sky Blue (`#38bdf8`).
- **Glassmorphic Panels**: Frosted container rendering, hairline borders, soft glow effects, and typography hierarchy.

### Phase 2: Fundamental Meter Modules
- **Peak / RMS Meter**: True peak sample tracking with continuous RMS averaging.
- **Classic VU Meter**: Initial analog needle emulation.
- **LUFS Meter**: K-weighted ITU-R BS.1770-4 loudness filters.

### Phase 3 & 4: Modular Grid Dashboard & Layout Engine
- **Dynamic Layout Engine**: CSS-grid auto-flowing container with single-click module addition, deletion, and Focus Mode zoom.
- **Component Lifecycle**: `MeterModule` base class governing paint bounds, header chrome, and frame rate budgeting.

### Phase 5: Advanced Visualizers & Analytical Suites
- **Spectrum Analyzer (5.4)**: 2048-point log-scaled FFT (20Hz–20kHz) with peak-hold ballistics and illuminated contours.
- **Phase Scope (5.3)**: Goniometer radar with Lissajous decaying persistence, polar graticule, and phase correlation readouts.
- **5-Minute Loudness Histogram (5.5)**: Continuous rolling distribution graph with modal loudness peaks.

### Phase 6: Visual Redesign & Polish
- **Multi-Pointer LUFS Dial (6.4)**: Swept 270° dial simultaneously displaying Integrated, Short-Term, and Momentary loudness.
- **Spring-Damped VU Needle (6.3)**: Physical second-order spring damper with smooth rotational inertia.
- **Responsive Bounding Fixes**: Aspect-ratio preserving gauge geometry across arbitrary window sizes.

### Phase 7: Mastering & Workflow Suite
- **7.1 Target Profile Presets**: Built-in broadcast & streaming targets with `PASS`/`HIGH`/`LOW` compliance badges.
- **7.2 Saveable Dashboard Layouts**: Factory suites + custom layout saving to APVTS state.
- **7.3 Session Report Export**: One-click export to CSV or branded HTML mastering deliverables.
- **7.4 A/B Loudness History Compare**: Snapshot capture + dual-pass A/B compare overlay.
- **7.5 Adaptive Frame Rate & CPU Budgeting**: Proportional auto-throttling under heavy load.
- **7.6 Colorblind-Safe Palette Toggle**: Accessibility switch routing warning colors to high-contrast `#38bdf8`.
- **7.7 Detachable Floating Windows**: Always-on-top pop-out windows with seamless re-docking.

### Phase 8: VU Ballistics Tuning & Segmented LED Meters
- **8.1 Fast-Response VU Ballistics**: Asymmetric attack/release (120ms rise / 350ms decay) with stiffened needle physics.
- **8.2 Segmented LED Level Meters**: ~48 discrete segments with dim unlit visibility, Gold/Amber-Red threshold coloring, and −3 dB threshold markers.
- **8.3 Refined VU Faceplate**: Tick marks from −20 to +3 VU with warning zone confined above 0 VU.

### Phase 9: VU Accuracy & Standalone Loopback Capture
- **9.1 VU Double-Smoothing Resolution**: Eliminated secondary SpringDamper latency; direct 1:1 rotation from DSP values. Replaced rectified peak with true continuous windowed RMS power integration.
- **9.2 Adjustable VU Reference**: APVTS-backed calibration combobox with presets: −18 dBFS (SMPTE), −20 dBFS (EBU), −14 dBFS (Streaming), −12 dBFS (Commercial), −10 dBFS (Club).
- **9.3 Standalone Loopback Capture**: `AudioSettingsModal` wrapping `juce::AudioDeviceSelectorComponent`, real-time I/O status badge, OS-specific loopback routing guides and device detection.

### Phase 10.4 Hotfix -- Phase Scope Regression (Beta v1.2.1)

**Regressing commit**: Phase 10 WASAPI freeze fix (same session, 07:00 CDT)
**Reported**: Immediately after first run post Phase 10 build
**Fixed**: 07:23 CDT, August 14, 2026

**Root Cause Analysis:**

The Phase 10.4 fix moved samplePairs.clear() from inside updateScopeImage() to the end
of timerCallback(), reasoning that updateScopeImage() bails early on zero-size bounds
(the WASAPI freeze cause) preventing the clear from ever running.

The flaw: repaint() in JUCE is **asynchronous**. It marks the component dirty and schedules
a paint event for the next message-loop iteration. Placing clear() immediately after
repaint() in the same synchronous timerCallback() means the vector is empty *before*
paint() -> updateScopeImage() ever executes. Result: scope showed nothing on every frame.

**Correct Fix:**

| Location | Change |
|---|---|
| timerCallback() | Removed samplePairs.clear() -- 8192-pair cap remains for runaway protection |
| updateScopeImage() zero-bounds early-return | Added explicit samplePairs.clear() -- drains accumulation on WASAPI startup frames |
| updateScopeImage() end of draw loop | Restored samplePairs.clear() -- runs after data is consumed, correct timing |

**Design rule documented in code:**
> Clear must happen after updateScopeImage() has consumed the data, not in
> timerCallback() where repaint() has not fired yet.

---

### Phase 10: Calibration Audit, Platform Stability & UI Consolidation ← **CURRENT**

#### 10.7 — Global Calibration Audit (COMPLETED)
Two root-cause DSP accuracy bugs were discovered and fixed:

**Bug 1 — `PeakRmsDSP` double-sqrt** *(Critical — affected all RMS readings)*
- `juce::AudioBuffer::getRMSLevel()` already returns the root-mean-square value.
- An extra `std::sqrt()` wrapper was computing `RMS^0.5`, making every RMS reading consistently hot.
- **Fix**: Removed the redundant `std::sqrt()` call. Single line change; immediately measurable accuracy improvement.

**Bug 2 — `VuDSP` AES-17 ×2 bias** *(Critical — added phantom +3 dB to all VU readings)*
- A `* 2.0f` multiplier was compounding on top of the already-hot RMS from Bug 1.
- Resulted in VU readings running ~+6 dB above true signal level in some configurations.
- **Fix**: Removed the multiplier; VuDSP now uses plain `sqrt(statePower)` = true RMS.

**Written Source of Truth** — `CalibrationTest.h`:

| Signal | Meter | Expected | Tolerance |
|---|---|---|---|
| 0 dBFS sine | Peak | 0.00 dBFS | ±0.25 dB |
| 0 dBFS sine | RMS | −3.01 dBFS | ±0.25 dB |
| −18 dBFS sine | VU (ref=−18) | 0.00 VU | ±0.25 dB |
| −20 dBFS sine | Peak | −20.00 dBFS | ±0.25 dB |
| −20 dBFS sine | RMS | −23.01 dBFS | ±0.25 dB |
| −20 dBFS sine | VU (ref=−18) | −5.01 VU | ±0.25 dB |

`CalibrationTestRunner::runAndLogFullSuite()` generates reference sine tones at 48kHz and routes them through PeakRmsDSP and VuDSP. Accessible via `CAL TEST` button in debug builds.

#### 10.4 — Phase Scope WASAPI Freeze (COMPLETED)
- **Root cause**: `samplePairs.clear()` lived inside `updateScopeImage()`, which guards with `if (w <= 0) return`. On Windows/WASAPI, audio callbacks arrive before `resized()` propagates, causing the guard to fire every frame — the vector grew unbounded, and the display froze while audio continued.
- **Fix**: Moved `samplePairs.clear()` to `timerCallback()` (unconditional) + 8192-pair hard accumulation cap as belt-and-suspenders protection.
- **Polish**: Persistence trail reduced 0.82 → 0.75 alpha for snappier feel.

#### 10.1 — Top Nav Bar Consolidation (COMPLETED)
- Replaced 7-button flat nav bar with: `⚙` Settings menu | Add Module combo | Layout combo | two `●` status dots.
- Settings menu contains all former standalone buttons plus UI Size submenu and About dialog.
- Status dots: cyan (live input) / amber (idle) / red (no input) for I/O; gold (60 FPS) / amber (throttled) for perf.

#### 10.2 — Per-Module Header Consolidation (COMPLETED)
- Replaced three separate header buttons (X / [] / [^]) with a single `⋮` kebab per module.
- Implemented once in `MeterModule` base class; all 6 module types inherit automatically.

#### 10.3 — VU Arc Scale Density (COMPLETED)
- Added 5 new graduated tick marks: −15, −12, −8, −6, −4 VU.
- Major label set expanded to include −15 and −12 for easier reading at typical program levels.

#### 10.5 — Spectrum Analyzer Upgrades (COMPLETED)
- Dual L/R channel traces: Left = full gold with gradient fill; Right = desaturated gold-gray, rendered behind.
- FFT resolution selector in module header: Low (1024) / Medium (2048) / High (4096).
- Decay time constant increased 0.1s → 0.18s for smoother peak hold.
- 3-bin moving-average smoothing applied post-FFT to reduce inter-bin jaggedness.

#### 10.6 — Unlit Segment Opacity (COMPLETED)
- `kUnlitAlpha = 0.03f` — halved from 0.06f.
- Extracted to a named constant for single-point maintenance.

#### 10.8 — Beta Version Labeling (COMPLETED)
- About dialog now shows `Beta v1.2.1 (Aug 14 2026)`.
- `README.md` and progress report updated with beta versioning.

#### 10.9 — WASAPI Loopback Surfacing (COMPLETED — confirmed shipped in Phase 9.3)
- `inspectPlatformLoopback()` in `AudioSettingsModal` already scans input device names for loopback indicators.
- Platform-specific guidance text updated to reference ⚙ Settings → Audio I/O Settings.

---

## Installation & Deployment Guide

### Windows 10/11 (x64) — Beta v1.2.1

> **Beta:** This release is pre-release software. Verify calibration via the `CAL TEST` button in the VU module (debug build) or against a known reference signal before use in production sessions.

1. Extract `FF360Meter_v1.2.0-beta_Windows_x64.zip`.
2. Copy `FF360Meter.vst3` to your system VST3 directory:
   ```text
   C:\Program Files\Common Files\VST3\
   ```
3. To run standalone, launch `FF360Meter.exe`.
4. For system audio monitoring: open **⚙ Settings → Audio I/O Settings** and select a Stereo Mix / WASAPI loopback / VB-Cable input device.

### macOS 12 and Later (Apple Silicon & Intel) — Beta v1.2.1

> **Beta:** Universal Binary (arm64 + x86_64). Notarization not yet applied for this beta build. macOS may require manual security approval on first launch: **System Settings → Privacy & Security → Open Anyway**.

1. Extract `FF360Meter_v1.2.0-beta_macOS_Universal.zip`.
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

## Technical Specifications

| Feature | Specification |
|:---|:---|
| **Framework & Standard** | JUCE 8.0.4, C++20 |
| **Plugin Formats** | VST3, AU, Standalone |
| **Sample Rates** | 44.1 kHz, 48 kHz, 88.2 kHz, 96 kHz, 192 kHz |
| **Loudness Standards** | ITU-R BS.1770-4, EBU R128 |
| **VU Ballistics** | True Windowed RMS (x² → IIR → √), 1:1 tracking, −18/−20/−14/−12/−10 dBFS ref |
| **Calibration Tolerance** | ±0.25 dB against reference sine tones (`CalibrationTest.h`) |
| **Spectrum FFT** | 1024 / 2048 / 4096-point Hanning window, log scale (20Hz–20kHz) |
| **Spectrum Channels** | Dual L/R with independent traces |
| **Phase Scope** | Lissajous goniometer, 0.75 persistence alpha, 8192-pair accumulation cap |
| **Thread Safety** | 100% Lock-Free SPSC Circular FIFO Buffers |
| **Accessibility** | Built-in Protanopia/Deuteranopia High-Contrast Mode (⚙ Settings) |
| **System Loopback** | WASAPI Loopback / Stereo Mix / BlackHole with live ● status dot |
| **Nav Bar** | ⚙ menu + 2 status dots + Add Module + Layout preset |
