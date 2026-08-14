# ff360_labs Modular Audio Metering Plugin — Phase 10: UI Consolidation, Accuracy Audit & Bug Fixes

Prerequisite: Phases 0–9. This phase addresses a batch of usability, accuracy, and platform-specific issues found during real-world use. Grouped by theme; several items share root causes, called out where relevant.

Act as a senior C++ / JUCE audio software engineer. Implement the following.

---

### 10.1 — Top Nav Bar Consolidation
The nav bar currently overflows/overlaps text at smaller UI scales because too many discrete controls live there directly.
1. Add a **"Settings/Options" dropdown menu** in the top-left of the nav bar, consolidating: Audio I/O device selection, Export Report (7.3), Accessibility palette toggle (7.6), and the Focus/Grid mode switch (Phase 2/4) into menu entries rather than standalone buttons.
2. Add a **Full Screen** toggle to this menu.
3. Add a **UI Size** submenu with fixed steps: 50%, 75%, 100%, 125%, 150%, 175%, 200% — wire this into the Phase 5.7 scale-factor system already in place, don't build a second scaling path.
4. Add an **About** entry (version number, ff360_labs branding/logo, build info).
5. Move the **"Live I/O"** and **"Perf: 60 FPS"** status indicators (from Phase 9.3's device-state messaging and Phase 7.5's performance indicator, respectively) into this same menu rather than living as permanent top-bar text — keep only a minimal glanceable icon/dot in the bar itself if any visual indicator should remain persistent, and clarify in a tooltip what each one means.
6. This should reduce the persistent nav bar to: ff360_labs branding, the Settings/Options menu, the Add Module control, and the layout preset dropdown from Phase 7.2 — everything else moves into the menu.

### 10.2 — Per-Module Options Menu
1. Replace the individual Resize / Detach (7.7) / Exit (close) controls in each module's header with a single **dropdown/kebab menu** per module, consistent with the Phase 6.1 shared header chrome.
2. Menu entries: Resize (or a resize-handle toggle), Detach to Window (7.7), Close/Remove.
3. Keep this consistent across every module type — implement once in the shared `MeterModule` header logic (per 6.1.2) rather than per-subclass, so new module types automatically inherit it.

### 10.3 — VU Meter: Deeper Accuracy Pass
Following up on Phase 9's fixes — if it's still reading hot/maxed in red for most playback, treat this as unresolved rather than assuming 9.1/9.2 fully fixed it.
1. Re-verify the RMS windowing math end-to-end with a **known reference tone test**: feed a calibrated -20 dBFS sine wave and confirm the VU reading lands at the expected calibrated position (e.g. -2 VU if reference is -18 dBFS) — if it doesn't, the bug is in the RMS/dB conversion chain, not the ballistics or calibration constant.
2. Add more graduated level numbers along the VU arc scale (e.g. fill in -15, -12, -8, -6, -4, -2 alongside the existing major ticks from Phase 8.3) so the current position is easier to read at a glance rather than only having ticks at the major values.
3. Consider whether the default calibration reference (9.2) should ship at -14 dBFS or similar instead of the broadcast-standard -18 dBFS default, given most real-world source material run through this meter will be modern, loud, streaming-mastered content — document the tradeoff either way rather than silently picking one.

### 10.4 — Phase Scope: Windows-Specific Freeze
This is a platform-specific bug, not a design issue — treat the "faster/snappier" request as secondary to fixing the freeze first.
1. Investigate the Phase 5.3 `PhaseScopeDSP` FIFO consumption specifically on the WASAPI backend — a scope that freezes after running for a while while other modules keep working typically points to a **FIFO read/write desync or an unhandled buffer wraparound** that only manifests under WASAPI's callback timing, not CoreAudio's.
2. Check for any place the Phase Scope's persistence/trail buffer (5.3.3) might silently stop advancing (e.g. an index that isn't wrapping correctly, or a buffer-full condition that isn't cleared) — this is a stronger suspect than the audio data itself stopping, since audio and other modules continue working fine.
3. Once the freeze is fixed, revisit responsiveness: reduce persistence trail length slightly and/or increase the module's refresh rate (per the Phase 7.5 per-module rate system) for a snappier feel, consistent with the "faster" pattern already applied to the VU meter in Phase 8.1.

### 10.5 — Spectrum Analyzer: Smoothing, Dual-Channel, Resolution
1. Smooth the rendered line/bars further — increase the peak-hold + decay smoothing from Phase 5.4.3, or apply a light moving-average across adjacent frequency bins, to reduce visual jaggedness without meaningfully increasing latency.
2. Add a **second overlapping trace** for the opposite channel: render L and R as two semi-transparent overlaid traces in slightly different tones (e.g. one at full gold, one at a dimmer/desaturated gold-gray) so both channels are visible simultaneously rather than only a summed/mono view.
3. Add a **resolution selector** exposing FFT order choices (e.g. 1024 / 2048 / 4096 from the Phase 5.4.1 `juce::dsp::FFT` setup) as a dropdown in the module header — higher orders trade latency/CPU for frequency resolution, so document that tradeoff briefly in the UI (e.g. a tooltip) rather than just exposing a bare number.

### 10.6 — Peak/RMS Meter: Unlit Segment Opacity
1. Reduce the opacity of unlit/inactive segments (Phase 8.2.4) by 50% from its current value — this should be a single constant change in the shared segment-rendering helper, not a per-module edit.

### 10.7 — Global Calibration Audit ("Everything Reads Hot")
This is likely the root cause tying together the VU complaint (10.3) and general mistrust of the readings.
1. Build a **built-in calibration/reference tone test mode** (dev-facing, can be hidden behind a settings toggle): generate a known sine tone at a known dBFS level internally and route it through each meter module (Peak/RMS, VU, LUFS) to confirm each one reports the mathematically correct value.
2. Audit specifically for a **peak-vs-RMS mixup** — a common bug class where a meter labeled "RMS" is actually computing/displaying a peak-rectified value, which would independently explain readings running consistently hotter than expected across multiple modules at once, rather than each module having an unrelated individual bug.
3. Once verified, document the expected reading for a 0 dBFS full-scale sine on each meter type (Peak should read 0, RMS of a sine should read ~-3 dBFS relative to peak, VU depends on calibration reference) so there's a written source of truth to check future changes against.

### 10.8 — Beta Version Labeling
1. Change the build/version display (surfaced in the new About menu from 10.1.4) to show **"Beta"** plus a build number or date, instead of a final-release-style version string, until you're ready to call it stable.

### 10.9 — WASAPI Loopback Confirmation
Already scoped in Phase 9.3.2 — flagging here since it appeared in the notes again. Confirm this shipped and is selectable in the Audio I/O settings now under the 10.1 Settings menu; if not yet done, treat 9.3 as still open rather than assuming it's complete.

---

### Suggested Sequencing
1. **10.7 (global calibration audit)** first — if there's a systemic peak/RMS mixup, it explains multiple other complaints (10.3) at once, so fixing it first may resolve issues that would otherwise look like separate bugs.
2. **10.4 (Phase Scope Windows freeze)** next — a functional bug on one platform should outrank cosmetic/UX polish.
3. **10.1 + 10.2 (nav consolidation)** together, since they're the same category of change and both fix the scaling/overlap problem.
4. **10.3, 10.5, 10.6, 10.8, 10.9** as a final polish pass once the above are confirmed fixed.

Please start with **10.7 (calibration audit)**, using a reference sine tone test, before touching any more UI code.
