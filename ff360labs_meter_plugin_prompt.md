# ff360_labs Modular Audio Metering Plugin — Build Prompt

Act as a senior C++ audio software engineer and JUCE expert.

I want to build a modular audio metering plugin structured under the "ff360_labs" brand identity. Functionally and structurally, model it after **Process.Audio's Decibel** metering plugin: a customizable canvas where users add, remove, resize, and rearrange independent meter modules (Peak/RMS, VU, LUFS, Phase Scope, Spectrum Analyzer, Histogram) rather than being locked into one fixed layout. Unlike Decibel, the **visual skin is NOT user-customizable** — it always renders in the fixed ff360_labs color scheme (deep slate / cyber cyan / signal orange) defined below. Think "Decibel's flexibility, ff360_labs' branding."

The plugin needs a dynamic layout manager that supports:
- **Dashboard Mode:** a responsive multi-module grid where modules can be resized and rearranged.
- **Focus Mode:** any single module expanded to fill the view, with one click back to Dashboard.

Please scaffold and build this project step-by-step across the following phases. Confirm and lock in Phase 0 decisions before writing any Phase 1 code.

---

### **Phase 0: Environment & Conventions**
1. Target **JUCE 8.x** (latest stable). Flag it clearly if any API used is version-sensitive.
2. Use **CMake** as the build system (not Projucer), structured for use in Visual Studio 2022 on Windows.
3. Plugin format targets: **VST3, AU, and Standalone** — Standalone so each phase can be auditioned without a DAW host.
4. Establish a `Source/` layout convention up front (e.g. `Source/GUI/`, `Source/DSP/`, `Source/Core/`) and stick to it across all phases.
5. Confirm project/company name fields (`ff360 Labs`, plugin code, bundle ID prefix) so JUCE metadata is consistent from the start.

---

### **Phase 1: Project Setup & Visual Styling System**
1. Initialize the JUCE audio plugin boilerplate per Phase 0 targets.
2. Create a central LookAndFeel class named `FF360LabsLookAndFeel` that inherits from `juce::LookAndFeel_V4`.
3. Establish the signature **ff360_labs** sleek, technical aesthetic:
   - **Background Dark:** Deep slate `#0D0F12`
   - **Container Dark:** Matte dark charcoal `#16191E`
   - **Accent Cyber Cyan:** Glowing highlight `#00E5FF`
   - **Accent Signal Orange:** Warning / Peak threshold `#FF6B00`
   - **Text / Indicators:** Clean off-white `#E0E6ED`
4. Style standard components (buttons, toggles, labels) to match this technical dashboard design.

---

### **Phase 2: Core Base Classes & Modular Dashboard Architecture**
1. Create a `MeterModule` class inheriting from `juce::Component` to act as the base class for all metering UI components.
   - Include standard layout padding, header bars, module titles, and a small "remove/close" affordance in the header (Decibel-style module management).
   - Give each module a `MeterModuleType` enum tag (e.g. `PeakRms`, `VU`, `LUFS`, `PhaseScope`, `Spectrum`, `Histogram`) so the dashboard can track what's active without caring about concrete subclasses.
2. Build a `MeterDashboard` layout manager class:
   - Use `juce::Grid` to dynamically lay out active modules in a responsive grid, supporting a variable number of modules (not just a fixed pair) so meter types can be added/removed at runtime.
   - Support **per-module resizing within the grid** (drag-resize handles on module edges, similar to Decibel's resizable windows) in addition to the overall responsive reflow.
   - Implement state management for `LayoutMode::Grid` (Dashboard view, all active modules) and `LayoutMode::Maximized` (Focus view, one module fills the canvas).
   - Handle transitions between layout modes using `juce::ComponentAnimator` for smooth resize/fade rather than an instant cut.
   - Keep an ordered `std::vector<std::unique_ptr<MeterModule>>` (or similar) as the source of truth so module add/remove/reorder stays simple to extend in later phases.

---

### **Phase 3: Real-Time Peak & RMS DSP Engine**
1. Implement a thread-safe, high-performance DSP engine for Peak and RMS detection inside `PeakRmsDSP.h`.
2. Implement ballistics smoothing (attack and exponential decay) for fluid UI meters running at high frame rates.
3. Write standard conversion utilities for linear gain to dBFS with a defined display range of **-60 dBFS (floor) to 0 dBFS (ceiling)**, with the floor value as a named constant so it's easy to change later.
4. Connect `processBlock` audio buffer data to the GUI thread using **`juce::AbstractFifo`** to pass level values from the audio thread to a GUI-thread `juce::Timer` callback — avoid raw shared atomics for anything beyond single scalar reads.

---

### **Phase 4: RMS/Peak Module UI Implementation & Wiring**
1. Create a concrete `PeakRmsMeterModule` inheriting from `MeterModule`.
2. Implement custom vector drawing in `paint()` using `FF360LabsLookAndFeel` colors:
   - Dual-channel (L/R) stereo level bars with smooth gradient fills (Cyan to Orange at -3dB threshold, using the -60 to 0 dBFS range from Phase 3).
   - Numerical dB readout overlays for Peak and RMS.
3. Instantiate and register meter modules inside `PluginEditor` to test the dashboard/module-management logic:
   - Module 1: Stereo Peak / RMS Meter (fully implemented, per above).
   - Modules 2–4: Lightweight placeholder `MeterModule` subclasses tagged `VU`, `LUFS`, and `PhaseScope` — enough to prove the dashboard handles multiple heterogeneous module types, without full DSP behind them yet.
4. Wire up a top navigation bar containing:
   - Layout toggle buttons (Grid View vs Focus View).
   - An "Add Module" control (dropdown or `+` button) that instantiates one of the registered placeholder types into the dashboard, mirroring Decibel's add/remove module workflow.
   - The `ff360_labs` header branding.

---

Please guide me through **Phase 0 and Phase 1** first. Confirm the environment setup, then provide the code for `FF360LabsLookAndFeel` and show how to apply it across the application.
