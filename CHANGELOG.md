# Changelog

All notable changes to the **ff360_labs Modular Audio Meter** are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to beta-stage [Semantic Versioning](https://semver.org/) (`0.x`/`Beta` releases may include breaking changes between minor versions).

## [Beta v1.3.0] — 2026-09-30

Phase 13 — house-style pass, bringing the meter in line with Apex360, Niche360 and the other ff360 Labs plugins: measurement fixes found by a new processor test suite, embedded brand fonts, tooltips, DPI-aware UI zoom, and the shared CI pipeline.

### Fixed — measurement accuracy
- **VU read ~1.2 dB hot on steady tones**: the attack / release ballistics ran per sample on instantaneous power (x²), which swings between zero and the peak twice per cycle, so the asymmetric integrator settled above the true mean. Power is now integrated symmetrically first (true RMS), then the ballistics act on that. A -20 dBFS sine reads -5.01 VU at the -18 dBFS reference, as `CalibrationTest.h` always claimed.
- **Integrated LUFS read ~0.2 LU low after every start or reset, and a steady tone showed ~9 LU of LRA**: the partial 400 ms / 3 s windows right after a reset were counted as gating blocks. Only complete windows count now (BS.1770-4 / EBU Tech 3342). EBU Tech 3341 case 1 (-23 dBFS stereo 1 kHz) reads -23.0 LUFS.
- **Mono input read 3 dB hot in LUFS**: a mono channel was counted as both L and R.
- **Spectrum read every tone 6 dB low**: the FFT magnitude was normalised by N/2, ignoring the Hann window's coherent gain. A 0 dBFS sine now reads 0 dB at every FFT size. The 3-bin smoothing is peak-preserving (a plain dB average pulled a pure tone down ~4 dB) and no longer feeds back into the peak-hold state, which blurred it further every frame.
- **Spectrum FFT resolution selector did nothing**: it had no handler. It now switches the analyser (without allocating on the audio thread) and is saved with the session.
- **Peak meter missed negative-going peaks**: it used the largest signed sample. It now uses the absolute magnitude.
- **Peak meter dropped peaks between UI frames**: only the newest audio block was drawn each frame, discarding the peaks of the other ~5-20 blocks. The meter now takes the highest peak (and power-averaged RMS) of every block since the last frame.
- **Session report exported the current, not maximum, short-term / momentary loudness, fixed -60 dB peaks, and always the first delivery target**: it now uses the session maxima, the highest sample peaks since the last reset, and the selected target.

### Fixed — stability and rendering
- **Phase Scope trail on Windows**: the fade (`multiplyAllAlphas`) was applied while a `Graphics` context was drawing into the same image. With JUCE 8's Direct2D images on Windows that isn't allowed (it asserted in debug builds) and the fade could be lost, leaving the trail smeared. The fade now happens before drawing, into a software image rendered at the physical pixel size. This is likely the underlying cause of the smear v1.2.3 addressed.
- **No allocation on the audio thread**: Phase Scope and Spectrum frames were `std::vector`s built and copied into the FIFO every block, and the spectrum's smoothing allocated per frame. Frames are fixed-size now and FIFO capacity is per queue; a test counts allocations inside `processBlock` (0 at 44.1 / 48 / 96 kHz, blocks of 32-4096).
- **LUFS RESET raced the audio thread**: it reset the DSP (buffers, filters, histograms) from the UI thread mid-block. Resets are requested atomically and performed by the audio thread; the readouts are atomics.
- **Audio settings dialog leaked**: closing it removed it from the editor without deleting it.
- **Current layout only reached the session when the editor closed**: it is saved as soon as modules or the layout mode change, so a host save with the window open keeps it.
- Only the main input bus is metered; the processor tolerates a zero-channel buffer.

### Added
- **Brand fonts**: Barlow Condensed (chrome, menus, dialogs, tooltips) and JetBrains Mono (readouts, scales), embedded as in Niche360 (SIL OFL 1.1), so text looks the same on every machine.
- **Tooltips on every control and module**, drawn in the house style. (The editor had tooltip text but no `TooltipWindow`, so none ever showed.)
- **DPI-aware UI zoom (75%-200%)**: the whole interface is laid out at (window size / zoom) and scaled, so text and meters stay sharp; detached module windows scale too. Zoom and window size are saved with the session. Replaces the UI Size menu, which only resized the window (and whose 50% step was below the minimum size). Resizing stays free-aspect, for wide or tall dashboards.
- **Double-click to reset** the LUFS meter (Integrated, LRA, session maxima) and the histogram.
- **Processor test suite** (`Tests/ProcessorTests.cpp`, 91 checks): sine calibration for peak, RMS, VU and LUFS at 44.1 / 48 / 96 kHz, EBU reference, mono, spectrum calibration per FFT size, correlation, resets, DEV OSC, pass-through, no allocations, and editor checks (tooltips, fonts, zoom, session restore, live layout saving). JUCE assertions and leaks fail the run. `--screenshot <png> [scale] [layout]` renders the editor; see `docs/screenshots`.

### Changed
- Meters repaint only when a value visibly changes (the VU and LUFS modules repainted 60 times a second even when idle); the editor is opaque.
- **CI** (matches Drum Bus and the global CI rules): Ninja on both platforms, the test suite runs on Windows and macOS, macOS bundles are signed ad hoc and validated with `auval`, zips are made with `ditto`, releases are tagged at the commit that was built and are prereleases by default, and a manual run without a version builds and tests without publishing. Release zips are named `FF360Meter_<tag>-beta_...`, matching the install instructions.
- `FF360_COPY_PLUGINS_AFTER_BUILD` (on locally, off in CI) controls installing the plugins after a build; `JUCE_DISPLAY_SPLASH_SCREEN=0`; VST3 category `Fx Analyzer`.
- The AU / VST3 codes (`F360` / `F301`) are unchanged so existing sessions keep loading the plugin, although the global rules now ask for letters-only codes.

---

## [Beta v1.2.3] — 2026-08-26

Phase 12 — Windows Phase Scope rendering fix and a UI style pass toward the reference dashboard mockup.

### Fixed
- **Phase Scope persistence trail smeared/overly-persistent on Windows**: `updateScopeImage()` faded the Lissajous trail by a fixed `×0.75` multiplier once per *paint call*, tuned assuming a steady ~60Hz cadence. `repaint()` only requests a paint — the OS decides when it actually runs — and this project enables no Direct2D or OpenGL context, so Windows falls back to JUCE's default unaccelerated software/GDI renderer, which services paints far less consistently under load than macOS's compositor. When paints landed slower than 60Hz, the trail decayed once per (longer, variable) real interval instead of once per 1/60s, fading much slower than intended in real time. The decay is now computed from actual elapsed wall-clock time (`PhaseScopeModule::lastDecayTimeMs`), so the fade rate stays constant regardless of how often the OS actually services the paint.

### Changed
- **Typography hierarchy**: introduced a dedicated sans-serif `getUiFont()` for interactive chrome (buttons, popup menus, toggle text, module header titles), separate from the monospace `getCustomFont()`/`getNumericReadoutFont()` used for technical data and readouts — mirroring the reference mockup's Inter/JetBrains Mono split, where the previous single "Consolas" font rendered every element identically (and silently failed to resolve at all on macOS, where Consolas doesn't exist).
- **Module header titles & app-bar subtitle** dimmed from bright bold white to a muted sans, matching the mockup's understated chrome — brightness and gold are now reserved for live data and the brand name, not structural chrome.
- **Dashboard grid gap** tightened from 4px to 2px for a denser panel layout closer to the mockup.
- Segmented LED meters, glassmorphic panel treatment, and gold hairline module borders are unchanged — these are established product identity, not mockup deviations.

---

## [Beta v1.2.2] — 2026-08-22

Phase 11 — silence diagnosis, real-time internal reference oscillator (DEV OSC), standalone input device selector, and module header vector icon encoding fix.

### Added
- **DEV OSC Calibrated Reference Generator (11.1)**: Built-in 1 kHz sine generator synthesized directly in `FF360MeterProcessor::processBlock` at -18 dBFS (0.12589 amp). Toggleable via a dedicated `DEV OSC` button in the top navigation bar. Instantly drives all meters across the suite to verified calibration targets for self-diagnosis.
- **Audio Input Device Selector (11.1)**: Top navigation bar dropdown in Standalone mode listing all available system input/loopback endpoints, applying changes dynamically to `AudioDeviceManager`.
- **4-State I/O Status Badge (11.1.3)**: Distinguishes between DEV OSC active (🟣 magenta), no hardware input device (🔴 red), input connected but idle/silent (🟡 amber), and active live audio streaming with live peak level readout (🟢 cyan).
- **MSVC `/utf-8` Compiler Flag**: Enforces UTF-8 execution character sets to guarantee multi-platform string and asset integrity.
- **Max for Live Accuracy & Diagnostic Guidelines (11.3–11.6)**: Comprehensive companion document establishing DEV OSC reference standards, dynamic `[adstatus sr]` sample-rate scaling, and BS.1770-4 dual-gate LRA rules for the parallel M4L suite.

### Fixed
- **Module Header Kebab Icon Encoding Bug (11.2)**: Replaced string-literal text buttons with native vector-rendered `KebabMenuButton` (`fillEllipse`), eliminating multi-byte encoding corruption (`â‹®`) and font glyph mismatch issues.
- **VU Scope Button Label**: Renamed local VU debug overlay toggle to `OVERLAY` to avoid confusion with the global `DEV OSC` audio reference generator.

---

## [Beta v1.2.1] — 2026-08-14

Hotfix release. Corrects a display regression introduced by the Phase 10.4 WASAPI freeze fix shipped earlier the same day.

### Fixed
- **Phase Scope blank / no display (regression)** — The Phase 10.4 fix moved `samplePairs.clear()` to run immediately after `repaint()` inside `timerCallback()`. Because `repaint()` is asynchronous in JUCE (it schedules a paint event rather than running one immediately), the sample buffer was empty by the time `updateScopeImage()` actually executed on the next message-loop iteration, so the scope rendered nothing every frame.
  - Removed the clear from `timerCallback()` (the 8192-pair hard accumulation cap remains as runaway protection).
  - Added an explicit `samplePairs.clear()` to the zero-bounds early-return path in `updateScopeImage()`, so accumulation is drained correctly during WASAPI startup frames.
  - Restored `samplePairs.clear()` to the end of the draw loop in `updateScopeImage()`, so the clear now runs only after the data has actually been consumed.
  - Documented the underlying rule in code comments: the buffer must be cleared after `updateScopeImage()` has consumed the data, not inside `timerCallback()` before `repaint()` has fired.

---

## [Beta v1.2.0] — 2026-08-14

Phase 10 — calibration audit, platform stability, and UI consolidation sprint. Two critical DSP accuracy bugs were found and fixed, bringing all meter readings into mathematical alignment with reference sine-tone expectations (±0.25 dB tolerance).

### Added
- **⚙ Settings menu**: consolidates Audio I/O, Export, Accessibility, Grid/Focus mode, UI Size (50%–200%), Full Screen, and About into a single top-left dropdown. Nav bar reduced to branding | ⚙ | Add Module | Layout preset | two status dots.
- **⋮ Per-module kebab menu**: replaces the three separate header buttons (X / `[]` / `[^]`) with a single options menu (Resize, Detach to Window, Close/Remove), implemented once in the `MeterModule` base class and inherited by all six module types.
- **Spectrum Analyzer dual L/R traces**: left channel in full gold with gradient fill, right channel in desaturated gold-gray rendered behind it.
- **Spectrum FFT resolution selector**: Low (1024) / Medium (2048) / High (4096), with a tooltip documenting the CPU/latency tradeoff.
- **VU arc scale density**: five new graduated tick marks (−15, −12, −8, −6, −4 VU); major label set expanded to include −15 and −12.
- **Built-in calibration test suite** (`CalibrationTest.h`): `CalibrationTestRunner::runAndLogFullSuite()` generates reference sine tones at 48 kHz and routes them through `PeakRmsDSP` and `VuDSP`, documenting expected readings for every meter type. Accessible via the `CAL TEST` button on the VU module in debug builds.
- **Beta version labeling**: About dialog now shows `Beta v1.2.0 (Aug 14 2026)`.
- **WASAPI loopback confirmation**: `inspectPlatformLoopback()` in `AudioSettingsModal` scans input device names for Stereo Mix / virtual cable / loopback indicators; platform guidance text now points to ⚙ Settings → Audio I/O Settings.

### Fixed
- **[Critical] `PeakRmsDSP` double-sqrt bug** — `juce::AudioBuffer::getRMSLevel()` already returns the RMS value; a redundant `std::sqrt()` wrapper was computing `RMS^0.5`, making every RMS reading run consistently hot. Removed the extra call.
- **[Critical] `VuDSP` AES-17 ×2 bias** — a `* 2.0f` multiplier was compounding on top of the already-hot RMS from the bug above, pushing VU readings up to ~+6 dB above true signal level in some configurations. Removed; `VuDSP` now uses plain `sqrt(statePower)` (true RMS).
- **[Platform] Phase Scope freeze on Windows/WASAPI** — `samplePairs.clear()` lived inside `updateScopeImage()`, which bails early on zero-size bounds. On WASAPI, audio callbacks can arrive before `resized()` propagates, so the guard fired every frame, the accumulation vector grew unbounded, and the display froze while audio kept playing. Fixed with an unconditional clear in `timerCallback()` plus an 8192-pair hard cap.
  - **Note:** this fix introduced the regression corrected in Beta v1.2.1 — see above.

### Changed
- Peak/RMS unlit segment opacity halved (`kUnlitAlpha` 0.06 → 0.03) for cleaner active-segment contrast.
- Phase Scope persistence trail alpha reduced 0.82 → 0.75 for a snappier feel.
- Spectrum decay time constant increased 0.1s → 0.18s, plus 3-bin moving-average smoothing applied post-FFT to reduce inter-bin jaggedness.

---

## [1.1.0] — 2026-08-13

First full-suite release. Establishes the core architecture, brand system, and mastering workflow feature set across Phases 0–9.

### Added
- **Core architecture**: lock-free SPSC circular `AudioFifo` channels isolating realtime DSP threads from 60 FPS GUI rendering.
- **Brand & design system**: `FF360LabsLookAndFeel` with Deep Black (`#0a0a0b`), Matte Charcoal (`#17171a`), Metallic Gold (`#c9a15a`), Warm Amber-Red (`#e8654a`), Accessible Sky Blue (`#38bdf8`); glassmorphic frosted panels, hairline borders, and typography hierarchy.
- **Fundamental meters**: Peak/RMS meter with true peak and continuous RMS averaging; classic analog-emulated VU meter; K-weighted ITU-R BS.1770-4 LUFS meter.
- **Modular grid dashboard**: CSS-grid-style auto-flowing layout engine with single-click module add/remove and Focus Mode zoom; `MeterModule` base class standardizing paint bounds, header chrome, and frame rate budgeting.
- **Advanced visualizers**: 2048-point log-scaled FFT Spectrum Analyzer (20Hz–20kHz) with peak-hold ballistics; Lissajous Phase Scope goniometer with polar graticule and correlation readouts; 5-minute rolling loudness histogram.
- **Visual polish**: swept 270° multi-pointer LUFS dial (Integrated / Short-Term / Momentary); spring-damped VU needle with second-order rotational inertia; aspect-ratio-preserving gauge geometry.
- **Mastering & workflow suite**: built-in broadcast/streaming target profile presets (Spotify, YouTube, Apple Music, Netflix, EBU R128, Club/Master, AES Streaming, Custom) with `PASS`/`HIGH`/`LOW` compliance badges; saveable factory + custom dashboard layouts via APVTS/`juce::ValueTree`; one-click CSV and branded HTML/PDF session report export; A/B loudness history compare with snapshot capture (`CAP A`); adaptive frame rate/CPU budgeting with live performance badge; colorblind-safe accessibility palette toggle; detachable always-on-top floating module windows with re-docking.
- **VU ballistics & segmented LED meters**: asymmetric attack/release ballistics (120ms rise / 350ms decay); ~48-segment discrete horizontal LED level meters with Gold/Amber-Red threshold coloring and −3 dB markers; refined VU faceplate with −20 to +3 VU tick marks.
- **VU accuracy pass**: eliminated a secondary `SpringDamper` latency stage so needle rendering executes 1:1 from DSP integration values; replaced rectified peak integration with true continuous windowed RMS power integration (x² → IIR → √) per AES-17 sine calibration; added a `DEV OSC` toggle for a live dual-trace oscilloscope HUD of DSP ballistics vs. needle angle.
- **Adjustable VU calibration reference**: `vuRefLevel` APVTS parameter with presets at −18 dBFS (SMPTE/US Broadcast), −20 dBFS (EBU), −14 dBFS (Streaming/Hot Master), −12 dBFS (Commercial Hot), and −10 dBFS (Club/High Level).
- **Standalone system audio (loopback) capture**: `AudioSettingsModal` wrapping `juce::AudioDeviceSelectorComponent`; real-time I/O status badge (`● LIVE I/O` / `● IDLE / SILENT` / `● NO INPUT`); platform-specific loopback routing guides for Windows (WASAPI/Stereo Mix), macOS (BlackHole 2ch + Multi-Output guide), and Linux (PipeWire monitor).

---

[Beta v1.2.3]: #beta-v123--2026-08-26
[Beta v1.2.1]: #beta-v121--2026-08-14
[Beta v1.2.0]: #beta-v120--2026-08-14
[1.1.0]: #110--2026-08-13
