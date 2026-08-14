# ff360_labs Modular Audio Metering Plugin — Phase 8: VU Ballistics Tuning & Segmented Meter Styling

Prerequisite: Phases 0–7. This phase does two things: (1) tightens VU response time, and (2) restyles the Peak/RMS bars and VU gauge toward a segmented, LED-style look — referencing the layout language of the reference screenshots (ribbed/segmented level bars, a linear tick-marked needle gauge with a compact digital readout beneath it) but rendered fully in the established ff360_labs gold/glass identity, not the blue reference skin.

Act as a senior C++ / JUCE audio software engineer. Implement the following.

---

### 8.1 — VU Ballistics: Faster Response
1. In `VuDSP.h`, expose the integration time constant from Phase 5.1 as an adjustable parameter rather than a fixed 300ms standard-VU constant.
2. Reduce the default rise time to roughly **100–150ms** (down from the standard 300ms VU spec) so the needle/readout reacts noticeably faster to transients, while keeping a touch of smoothing so it doesn't look jittery like a true peak meter.
3. Keep decay slightly slower than attack (e.g. attack ~120ms, release ~300–400ms) so the motion still reads as "analog-inspired," just snappier on the way up — this asymmetry is what keeps it feeling like a VU meter rather than a PPM.
4. Make both values named constants (not magic numbers) so they're easy to re-tune by ear once you hear it in context.

### 8.2 — Peak/RMS Meter: Segmented Bar Restyle
Replace the smooth gradient bar fill from Phase 6.2 with a **segmented, LED-style bar**:
1. Divide each channel bar into discrete horizontal segments (thin rounded rectangles stacked vertically) rather than one continuous fill — draw a fixed number of segments across the dB range (e.g. one segment per 1–2 dB) and light up only the segments below the current level, matching the ribbed/segmented look in your reference.
2. Segments below the -3dB threshold render in gold; segments at/above threshold render in amber-red — same threshold logic as Phase 6.2, just expressed as discrete segments instead of a gradient.
3. Add a distinct **threshold marker line** at the -3dB point, rendered as a brighter horizontal accent segment (like the red line in your reference image) rather than only implied by the color change — this gives a clear "ceiling" reference at a glance.
4. Unlit segments should still be faintly visible (very low-opacity gold/charcoal) rather than fully invisible, so the full scale is always legible — matching the visible-but-dim unlit segments in the reference.
5. Keep the glow-on-active-fill treatment from Phase 6.2, but apply it per-segment (glow strongest on the topmost lit segment, tapering below) rather than as one blur across the whole bar — this reads more like real LED metering hardware.

### 8.3 — VU Gauge: Tick-Marked Arc Restyle
Refine the Phase 6.3 arc gauge to match the tick-marked linear/arc gauge style in your reference:
1. Draw explicit numbered tick marks along the arc (e.g. -20, -10, -7, -5, -3, -1, 0, 1, 2, +) rather than a plain unmarked arc — gold ticks, warm off-white numerals, larger ticks at major values.
2. Keep the amber-red zone confined to the tick marks at/above 0, matching the reference's small red arc segment rather than a large redline sweep.
3. Keep the glowing gold needle and spring-damped motion from Phase 6.3, now driven by the faster 8.1 ballistics — the visual and the response speed should be tuned together, since a fast needle on a laggy faceplate (or vice versa) will look wrong.
4. Keep the compact digital readout tile beneath the arc (e.g. "-0.0") from Phase 6.3 — this matches the reference layout already.

### 8.4 — Verification Pass
1. A/B the new VU response against the old 300ms version with a transient-heavy source (drums, percussive material) to confirm the snappier feel is actually noticeable and not overshooting/jittering.
2. Confirm segment count on the Peak/RMS bars stays legible at both minimum and maximum window size from the Phase 5.7 scalability work — segments that are too thin at small sizes should merge/reduce count rather than become illegible slivers.

---

Please start with **8.1 (VU ballistics)** since it's the smaller, faster change, then move to **8.2 (segmented Peak/RMS bars)**.
