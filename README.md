# ff360_labs Modular Audio Meter

A professional, modular audio metering and mastering analysis suite built with C++20 and JUCE 8.

---

## Overview
**ff360_labs Modular Audio Meter** is an extensible, multi-module audio analysis plugin designed for mixing, mastering, and broadcast compliance workflows. Built on a glassmorphic dark-mode design system with metallic gold accents, it features a dynamic grid dashboard, hardware-styled segmented LED meters, analog-modeled VU ballistics, and exportable session reports.

---

## Key Features & Metering Modules

### 1. Dynamic Modular Dashboard
- **Grid & Focus Layouts**: Auto-flowing responsive CSS-grid style layouts and single-module focus mode.
- **Factory Layout Presets**: Built-in suites (*Mastering*, *Broadcast QC*, *Quick Check*, *Full Suite*) with custom layout saving and APVTS session recall.
- **Detachable Floating Windows**: Pop out any module (`[^]`) into an always-on-top, resizable floating window with seamless re-docking on close.

### 2. Metering Suite
- **Peak / RMS Meter**: Hardware-inspired discrete segmented LED level bars (~48 segments) with illuminated topmost glow, core RMS luminance, and horizontal -3dB threshold markers.
- **VU Meter**: Analog-modeled arc gauge with true windowed RMS power integration ($x^2 \to \text{IIR} \to \sqrt{\cdot}$), 1:1 needle tracking (0ms secondary lag), adjustable calibration reference levels (-18, -20, -14, -12, -10 dBFS), and real-time Dev Timing Oscilloscope (`DEV OSC`).
- **LUFS Meter (ITU-R BS.1770-4)**: Multi-pointer circular dial displaying Integrated, Short-Term, and Momentary loudness, accompanied by target compliance stat-tiles (`PASS`, `HIGH`, `LOW`).
- **Target Profile Presets**: Built-in platform targets for Spotify (-14 LUFS), YouTube (-14 LUFS), Apple Music (-16 LUFS), Netflix (-27 LUFS), EBU R128 (-23 LUFS), Club/Master (-9 LUFS), AES Streaming (-16 LUFS), and Custom targets.
- **Spectrum Analyzer**: 2048-point log-scaled FFT analyzer (20Hz–20kHz) with peak-hold ballistics and illuminated spectral contours.
- **Histogram (5-Min Rolling & A/B Compare)**: Real-time loudness distribution histogram with modal peak highlights, snapshot capture (`CAP A`), and dual-pass A/B compare overlay with real-time modal delta readouts.
- **Phase Scope**: Goniometer radar with Lissajous decaying persistence traces, polar graticule, and correlation stat-tiles.

### 3. Workflow & Standalone Audio Capture
- **Standalone System Audio (Loopback) Capture**: In-app Audio I/O settings modal, real-time input status indicators (`LIVE I/O`, `IDLE / SILENT`, `NO INPUT`), and native OS loopback capture routing (Windows WASAPI/Stereo Mix, macOS BlackHole/Loopback guide, Linux PipeWire).
- **Session Report Export**: One-click export of session metrics (LUFS-I, LRA, Short-Term Max, Peak L/R, True Peak) as Spreadsheet CSV or branded print-to-PDF HTML mastering deliverables.
- **Adaptive Frame Rate & Performance Budgeting**: Per-module target refresh rates with automated load monitoring (`PERF: 60 FPS` / `PERF: 45 FPS`).
- **Colorblind-Safe Accessibility Palette**: Centralized toggle swapping warning tones for high-contrast Accessible Sky Blue (`#38bdf8`) for deuteranopia/protanopia compliance.

---

## Architecture
- **Lock-Free Concurrency**: Audio DSP algorithms run entirely lock-free on the realtime audio thread, publishing frames to GUI visualizers via circular `AudioFifo` buffers.
- **Single Source of Timing Truth**: DSP performs exact mathematical ballistics integration while visualizers render 1:1 directly, preventing stacked smoothing latency.
- **Centralized LookAndFeel**: The entire interface is driven by `FF360LabsLookAndFeel` with custom typography, frosted glass panels, hairline metallic gold borders, and stat-tile components.
- **Full State Serialization**: Plugin layouts, calibration levels, active target profiles, and accessibility preferences persist via `juce::AudioProcessorValueTreeState` and `juce::ValueTree`.

---

## Build Instructions
```bash
# Generate build files with CMake
cmake -B build -S .

# Build the plugin (VST3 and Standalone)
cmake --build build --config Release
```

### System Requirements
- JUCE 8.x
- CMake 3.22+
- C++20 compliant compiler (MSVC 2022, Clang, or GCC)

