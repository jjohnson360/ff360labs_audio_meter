# ff360_labs Modular Audio Metering Plugin — Phase 7: Next-Level Features

Prerequisite: Phases 0–6 complete (architecture, DSP modules, glassmorphic gold visual identity). This phase adds product-grade features on top of the existing module catalog and dashboard — no changes to the core `MeterModule`/`MeterDashboard` contract should be needed if earlier phases held to their base-class contracts.

Act as a senior C++ / JUCE audio software engineer. Implement the following, each as an independently shippable increment.

---

### 7.1 — Target Profile Presets
1. Define a `LoudnessTarget` struct (name, target LUFS-I, tolerance range, max true peak) and a small built-in preset table: Spotify (-14 LUFS), YouTube (-14 LUFS), Apple Music (-16 LUFS), Netflix (-27 LUFS), EBU R128 Broadcast (-23 LUFS).
2. Add a preset dropdown to the `LufsMeterModule` header (replacing the two hardcoded targets from Phase 5.2) that drives the pass/fail color logic via the shared `getReadoutColour()` helper from Phase 1 Revision.
3. Persist the last-selected target in plugin state so it's recalled on reopen.

### 7.2 — Saveable Dashboard Layouts
1. Serialize the current `MeterDashboard` state (active module types, grid positions/sizes, Grid vs Focus mode) to a `juce::ValueTree`, storable as a named preset (e.g. "Mastering," "Broadcast QC," "Quick Check").
2. Add a layout preset dropdown in the top nav bar (alongside the Phase 4 Grid/Focus toggle) to save, rename, delete, and recall layouts.
3. Ship 2–3 sensible factory-default layouts so the plugin doesn't open empty on first launch.

### 7.3 — Session Report Export
1. Build a `SessionReport` data collector that captures, across a monitored session: final LUFS-I, LRA, true peak, and pass/fail against the active target (7.1).
2. Export as **PDF** (reuse the project's `pdf` skill/toolchain conventions if generating this from a companion script, or a lightweight in-plugin PDF writer if fully native) and as **CSV** for spreadsheet workflows.
3. Include the ff360_labs branding/wordmark and the gold/glass visual identity in the PDF header, so exported reports are recognizably on-brand deliverables for clients.
4. Add a "Export Report" action to the nav bar, consistent with the Add Module / layout preset controls already there.

### 7.4 — A/B Loudness History Compare
1. Allow capturing the current `HistogramModule` (Phase 5.5) session as a named snapshot ("A"), then continuing to monitor a second pass as "B."
2. Render both snapshots overlaid on the histogram and timeline view, A in gold, B in a distinguishable secondary tone (not the amber-red warning color, to avoid implying B is "bad") — consider a cool-toned neutral like a desaturated blue-gray so it reads as "comparison," not "warning."
3. Add a small summary delta readout (LUFS-I difference, true peak difference) between A and B.

### 7.5 — Adaptive Frame Rate / CPU Budget
1. Give each `MeterModule` subclass a declared target refresh rate (e.g. Spectrum Analyzer 30fps, Phase Scope 30fps, numeric-readout-only modules 15fps) rather than one global timer rate for everything.
2. Add a lightweight CPU/frame-time monitor; if the dashboard is under load with many modules active simultaneously, throttle the declared rates proportionally rather than dropping frames unevenly.
3. Surface a simple performance indicator (small dot or readout, gold = healthy, amber-red = throttling active) in the nav bar for transparency, styled per the Phase 6 identity.

### 7.6 — Colorblind-Safe Palette Toggle
1. Add a secondary palette variant swapping amber-red for a blue-toned contrast pair, for deuteranopia/protanopia accessibility (~8% of male users).
2. Route this through the same centralized `Palette`/`Colours` namespace established in the Phase 1 Revision so no module needs individual changes — confirm this was in fact centralized correctly if this step requires touching more than one file.
3. Add the toggle to plugin settings, persisted in plugin state.

### 7.7 — Detachable Module Windows
1. Allow a module to "pop out" into its own floating `juce::DocumentWindow`, driven by a control in the module header (per Phase 6.1's shared chrome).
2. Popped-out modules should continue receiving data via the existing Phase 3 FIFO pipeline — no duplicate DSP or data path.
3. Closing the floating window returns the module to its prior dashboard position; this should reuse the existing add/remove module logic from Phase 4/5.6 rather than introducing a second module lifecycle path.

---

### Suggested Sequencing
1. **7.1 + 7.2** first — presets and saveable layouts are the highest-value, lowest-risk additions and make the plugin feel finished day-to-day.
2. **7.3** next — report export is a strong differentiator for client-facing/mastering use and a good portfolio artifact in its own right.
3. **7.5 + 7.6** as a polish pass once the feature set above is stable.
4. **7.4** and **7.7** last — both are more involved (state snapshotting, window lifecycle management) and are less essential to daily use than the above.

Please guide me through **7.1 (Target Profile Presets)** first, since 7.3's report export depends on it.
