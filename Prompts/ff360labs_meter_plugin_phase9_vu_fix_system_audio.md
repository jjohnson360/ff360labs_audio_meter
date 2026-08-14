# ff360_labs Modular Audio Metering Plugin — Phase 9: VU Response Fix & Standalone System Audio Capture

Prerequisite: Phases 0–8. This phase fixes a likely double-smoothing bug behind the still-sluggish VU response, exposes VU calibration as an adjustable control, and adds real system-audio capture to the Standalone build (which currently defaults to microphone input, not system playback).

Act as a senior C++ / JUCE audio software engineer. Implement the following.

---

### 9.1 — Fix VU Double-Smoothing
1. Audit `VuDSP.h` (Phase 8.1) and `VuMeterModule`'s needle-motion code (Phase 6.3) for **two separate smoothing stages** acting on the same value: DSP-side ballistics integration, and a spring/damping easing applied again during needle rendering.
2. Establish **one single source of timing truth**. Recommended split: DSP does the ballistics-accurate level integration (8.1's ~100-150ms attack), and the needle rendering does pure 1:1 rotation from that value with **no additional easing** — or, if visual easing is kept for a "premium" feel, remove ballistics smoothing from the DSP layer entirely and let the needle's spring constants alone define the response time. Pick one; do not stack both.
3. If keeping visual spring easing, tune its constants to be fast enough (critically damped or slightly underdamped, low settle time) that it doesn't reintroduce the sluggishness Phase 8.1 was meant to fix.
4. Add a debug overlay toggle (dev-only, not shipped) that plots the raw DSP output value against the rendered needle position over time, so response time can be visually verified rather than judged by ear alone.

### 9.2 — Expose VU Calibration Reference
1. Add a calibration reference level control (default **-18 dBFS = 0 VU**, the broadcast standard) as an adjustable setting rather than a hardcoded constant, with common presets: -18 dBFS (broadcast standard), -20 dBFS (EBU), -14 dBFS (streaming-loudness-informed, for users monitoring hot masters without the needle living in the red).
2. Surface this in the module's settings, alongside the existing target-profile presets from Phase 7.1, since both are "what's normal" reference questions.
3. Verify the underlying RMS calculation itself (Phase 3/5.1 math) as part of this pass — confirm it's a true windowed RMS and not, e.g., a rectified-peak value mislabeled as RMS, which would independently cause it to read hotter than expected regardless of calibration.

### 9.3 — Standalone: System Audio (Loopback) Capture
This is fundamentally a device-selection and platform-capability problem, not a DSP problem — scope accordingly.
1. In the Standalone target's audio settings panel, extend the existing `juce::AudioDeviceSelectorComponent` (or custom equivalent) to clearly list **input** devices separately from the default mic, so a loopback device is selectable once present on the system.
2. **Windows:** WASAPI loopback capture is natively available — add a toggle or dedicated entry ("System Audio (Loopback)") that opens the default output device in loopback mode via JUCE's WASAPI backend, rather than requiring a third-party virtual cable.
3. **macOS:** there is no built-in OS loopback; document this clearly in-app rather than silently failing. Detect whether a virtual loopback driver (e.g. BlackHole) is installed and selectable as an input; if none is found, show an in-app message pointing the user to install one and route system output through it (this is an OS-level requirement, not something the plugin can bypass).
4. **Linux:** detect PipeWire/PulseAudio monitor sources and surface them as selectable inputs, similar to the Windows loopback entry.
5. Add a clear "no system audio detected" state to any meter module when the selected input is silent/disconnected, so the UI doesn't just look broken — differentiate visually from "silence in the actual audio" vs. "no valid input device selected."

### 9.4 — Verification Pass
1. Confirm 9.1's fix with the same transient-heavy A/B test from Phase 8.4 — response should now visibly track kick/bass transients rather than smearing them.
2. Test Standalone loopback capture against real playback (e.g. a browser tab or media player) on your primary OS first, then confirm the "no device found" messaging path works correctly by temporarily deselecting the input.

---

Please start with **9.1 (fix double-smoothing)** since that's almost certainly the root cause of the sluggishness, then **9.3 (system audio capture)** since that's the functional blocker for actually using the standalone build day-to-day.
