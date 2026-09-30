// Processor-level tests: runs the real FF360MeterProcessor offline and checks what the meters
// report against reference signals (peak, RMS, VU, LUFS, spectrum, correlation), that
// processBlock never allocates, and that the editor's tooltips, zoom and session state
// behave. Exits non-zero on any failure.
//
// --screenshot <file.png> [scale] [layout] renders the editor with signal through it.

#include "../Source/Core/PluginProcessor.h"
#include "../Source/GUI/PluginEditor.h"

#include <atomic>
#include <typeinfo>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <new>

//==============================================================================
// Allocation counting: every operator new on a thread that has counting switched on is counted
namespace
{
    thread_local bool countAllocations = false;
    std::atomic<int> allocationCount { 0 };
}

void* operator new(std::size_t size)
{
    if (countAllocations)
        ++allocationCount;
    if (void* p = std::malloc(size == 0 ? 1 : size))
        return p;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) { return operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

//==============================================================================
namespace
{
    int failures = 0;
    int checks = 0;

    void check(bool ok, const juce::String& what)
    {
        ++checks;
        if (!ok)
        {
            ++failures;
            std::cout << "  FAIL: " << what << std::endl;
        }
    }

    void checkNear(float actual, float expected, float tolerance, const juce::String& what)
    {
        check(std::abs(actual - expected) <= tolerance,
              what + ": " + juce::String(actual, 3) + " (expected " + juce::String(expected, 3)
                  + " +/- " + juce::String(tolerance, 2) + ")");
    }

    float dbToGain(float db) { return std::pow(10.0f, db / 20.0f); }

    struct Rig
    {
        // On the heap, as in a host: the processor's fixed-size frame queues are ~0.5 MB
        std::unique_ptr<FF360MeterProcessor> procHolder = std::make_unique<FF360MeterProcessor>();
        FF360MeterProcessor& proc = *procHolder;
        double sampleRate = 48000.0;
        int blockSize = 480;
        long long sampleIndex = 0;

        void prepare(double sr, int block, bool mono = false)
        {
            sampleRate = sr;
            blockSize = block;

            auto layout = proc.getBusesLayout();
            const auto set = mono ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo();
            layout.inputBuses.getReference(0) = set;
            layout.outputBuses.getReference(0) = set;
            const bool ok = proc.setBusesLayout(layout);
            jassert(ok);
            juce::ignoreUnused(ok);

            proc.setRateAndBufferSizeDetails(sr, block);
            proc.prepareToPlay(sr, block);
            sampleIndex = 0;
        }

        // Runs `seconds` of signal through the processor; gen(n, ch) gives each sample
        void run(double seconds, const std::function<float(long long, int)>& gen,
                 std::function<void(const juce::AudioBuffer<float>&)> afterBlock = {})
        {
            const int channels = proc.getTotalNumInputChannels();
            juce::AudioBuffer<float> block(channels, blockSize);
            juce::MidiBuffer midi;
            const int numBlocks = (int) std::ceil(seconds * sampleRate / blockSize);

            for (int b = 0; b < numBlocks; ++b)
            {
                for (int ch = 0; ch < channels; ++ch)
                    for (int i = 0; i < blockSize; ++i)
                        block.setSample(ch, i, gen(sampleIndex + i, ch));
                sampleIndex += blockSize;

                proc.processBlock(block, midi);
                if (afterBlock)
                    afterBlock(block);
            }
        }

        std::function<float(long long, int)> sine(float peakDb, double freq, bool invertRight = false) const
        {
            const float amp = dbToGain(peakDb);
            const double sr = sampleRate;
            return [amp, sr, freq, invertRight](long long n, int ch)
            {
                const float s = amp * (float) std::sin(juce::MathConstants<double>::twoPi * freq * (double) n / sr);
                return (invertRight && ch == 1) ? -s : s;
            };
        }
    };

    template <typename T, int N>
    T drainLatest(AudioFifo<T, N>& fifo)
    {
        T data {};
        fifo.pullLatest(data);
        return data;
    }
}

//==============================================================================
static void testSineCalibration()
{
    std::cout << "Sine calibration (peak, RMS, VU, LUFS)" << std::endl;

    for (double sr : { 44100.0, 48000.0, 96000.0 })
    {
        Rig rig;
        // 1 kHz fits a whole number of cycles in the block, so each block's RMS is exact
        rig.prepare(sr, (int) (sr / 100.0));
        rig.run(4.0, rig.sine(-20.0f, 1000.0));

        const juce::String at = " @ " + juce::String(sr / 1000.0, 1) + " kHz";
        const auto meter = drainLatest(rig.proc.meterFifo);
        checkNear(meter.peakL, -20.0f, 0.05f, "Peak L of a -20 dBFS sine" + at);
        checkNear(meter.peakR, -20.0f, 0.05f, "Peak R of a -20 dBFS sine" + at);
        checkNear(meter.rmsL, -23.01f, 0.05f, "RMS L of a -20 dBFS sine" + at);

        // VU at the default -18 dBFS reference
        const auto vu = drainLatest(rig.proc.vuFifo);
        checkNear(vu.vuL, -5.01f, 0.1f, "VU of a -20 dBFS sine, ref -18" + at);

        // A stereo sine's loudness equals its peak level in dBFS (EBU Tech 3341 case 1 is -23 -> -23 LUFS)
        checkNear(rig.proc.lufsDSP.getMomentary(),  -20.0f, 0.1f, "Momentary LUFS of a stereo -20 dBFS sine" + at);
        checkNear(rig.proc.lufsDSP.getShortTerm(),  -20.0f, 0.1f, "Short-term LUFS of a stereo -20 dBFS sine" + at);
        checkNear(rig.proc.lufsDSP.getIntegrated(), -20.0f, 0.1f, "Integrated LUFS of a stereo -20 dBFS sine" + at);
        checkNear(rig.proc.lufsDSP.getLRA(), 0.0f, 0.2f, "LRA of a steady sine" + at);
    }
}

static void testEbuReference()
{
    std::cout << "EBU Tech 3341 case 1 (-23 dBFS stereo 1 kHz)" << std::endl;
    Rig rig;
    rig.prepare(48000.0, 512);
    rig.run(20.0, rig.sine(-23.0f, 1000.0));
    checkNear(rig.proc.lufsDSP.getIntegrated(), -23.0f, 0.1f, "Integrated");
    checkNear(rig.proc.lufsDSP.getMomentary(), -23.0f, 0.1f, "Momentary");
}

static void testMonoLoudness()
{
    std::cout << "Mono input" << std::endl;
    Rig rig;
    rig.prepare(48000.0, 480, true);
    rig.run(4.0, rig.sine(-20.0f, 1000.0));

    // One channel: half the power of the same sine on both sides (was read as stereo, 3 dB hot)
    checkNear(rig.proc.lufsDSP.getIntegrated(), -23.01f, 0.1f, "Integrated LUFS of a mono -20 dBFS sine");
    const auto meter = drainLatest(rig.proc.meterFifo);
    checkNear(meter.peakR, -20.0f, 0.05f, "Mono peak shown on both lanes");
}

static void testNegativePeaks()
{
    std::cout << "Negative-going peaks" << std::endl;
    Rig rig;
    rig.prepare(48000.0, 480);
    // Quiet positive signal with a single -0.5 spike per block
    rig.run(0.5, [](long long n, int) { return (n % 480 == 100) ? -0.5f : 0.01f; });
    const auto meter = drainLatest(rig.proc.meterFifo);
    checkNear(meter.peakL, -6.02f, 0.05f, "Peak of a -0.5 spike");
}

static void testSpectrumCalibration()
{
    std::cout << "Spectrum calibration" << std::endl;

    for (auto res : { FFTResolution::Low, FFTResolution::Mid, FFTResolution::High })
    {
        Rig rig;
        rig.proc.spectrumDSP.setFFTResolution(res);
        rig.prepare(48000.0, 512);

        const int fftSize = 1 << static_cast<int>(res);
        const int bin = fftSize / 16;
        const double freq = bin * 48000.0 / fftSize; // bin-centred

        for (float level : { 0.0f, -20.0f })
        {
            // Drain as the UI does, so the queue never fills
            SpectrumData frame;
            bool got = false;
            rig.run(2.0, rig.sine(level, freq), [&](const juce::AudioBuffer<float>&) { got = rig.proc.spectrumFifo.pullLatest(frame) || got; });
            check(got, "a spectrum frame arrives");
            check(frame.numBins == fftSize / 2, "bin count matches FFT size " + juce::String(fftSize));

            float peak = -200.0f;
            for (int i = 0; i < frame.numBins; ++i)
                peak = juce::jmax(peak, frame.magnitudesL[(size_t) i]);
            checkNear(peak, level, 0.1f, "spectrum peak of a " + juce::String(level, 0) + " dBFS sine, FFT " + juce::String(fftSize));
        }
    }
}

static void testFftResolutionSwitch()
{
    std::cout << "FFT resolution switching" << std::endl;
    Rig rig;
    rig.prepare(48000.0, 256);
    rig.run(0.5, rig.sine(-12.0f, 440.0));

    for (auto [res, bins] : { std::pair { FFTResolution::High, 2048 }, std::pair { FFTResolution::Low, 512 } })
    {
        rig.proc.spectrumDSP.setFFTResolution(res);
        SpectrumData frame;
        allocationCount = 0;
        countAllocations = true;
        rig.run(0.5, rig.sine(-12.0f, 440.0), [&](const juce::AudioBuffer<float>&) { rig.proc.spectrumFifo.pullLatest(frame); });
        countAllocations = false;
        check(frame.numBins == bins, "resolution switch reaches the audio thread (" + juce::String(bins) + " bins)");
        check(allocationCount.load() == 0, "switching resolution does not allocate");
    }
}

static void testCorrelation()
{
    std::cout << "Phase scope correlation" << std::endl;
    Rig rig;
    rig.prepare(48000.0, 4096);
    rig.run(1.0, rig.sine(-12.0f, 500.0));
    PhaseScopeData frame;
    rig.proc.phaseScopeFifo.pullLatest(frame);
    checkNear(frame.correlation, 1.0f, 0.01f, "identical channels correlate at +1");
    check(frame.numPoints > 0 && frame.numPoints <= PhaseScopeData::MaxPoints, "a 4096-sample block is decimated to at most 256 points");

    rig.run(3.0, rig.sine(-12.0f, 500.0, true));
    rig.proc.phaseScopeFifo.pullLatest(frame);
    checkNear(frame.correlation, -1.0f, 0.01f, "inverted channels correlate at -1");
}

static void testResetsAndSessionMaxima()
{
    std::cout << "Loudness reset and session maxima" << std::endl;
    Rig rig;
    rig.prepare(48000.0, 480);
    rig.run(3.0, rig.sine(-10.0f, 1000.0));
    rig.run(3.0, rig.sine(-30.0f, 1000.0));

    checkNear(rig.proc.lufsDSP.getMaxShortTerm(), -10.0f, 0.2f, "short-term maximum holds the loud passage");
    checkNear(rig.proc.getSessionPeakDb(0), -10.0f, 0.05f, "session peak holds the loud passage");

    // Requested from the UI thread; the audio thread performs it on its next block
    rig.proc.resetLoudnessSession();
    check(rig.proc.lufsDSP.getIntegrated() > -69.0f, "reset waits for the audio thread");
    rig.run(0.05, rig.sine(-30.0f, 1000.0));
    check(rig.proc.lufsDSP.getIntegrated() <= -69.0f, "integrated clears after reset");
    rig.run(3.0, rig.sine(-30.0f, 1000.0));
    checkNear(rig.proc.lufsDSP.getIntegrated(), -30.0f, 0.1f, "integrated re-measures from the reset");
    checkNear(rig.proc.getSessionPeakDb(0), -30.0f, 0.05f, "session peak re-measures from the reset");
}

static void testDevOsc()
{
    std::cout << "DEV OSC reference" << std::endl;
    Rig rig;
    rig.prepare(48000.0, 480);
    rig.proc.setDevOscEnabled(true);
    rig.run(4.0, [](long long, int) { return 0.0f; });
    rig.proc.setDevOscEnabled(false);

    const auto meter = drainLatest(rig.proc.meterFifo);
    checkNear(meter.peakL, -18.0f, 0.05f, "DEV OSC peak");
    const auto vu = drainLatest(rig.proc.vuFifo);
    checkNear(vu.vuL, -3.01f, 0.1f, "DEV OSC VU at the -18 dBFS reference");
    checkNear(rig.proc.lufsDSP.getIntegrated(), -18.0f, 0.1f, "DEV OSC integrated LUFS");
}

static void testPassThrough()
{
    std::cout << "Pass-through" << std::endl;
    Rig rig;
    rig.prepare(48000.0, 256);
    const auto gen = rig.sine(-6.0f, 330.0);
    float maxError = 0.0f;
    long long start = 0;
    rig.run(0.5, gen, [&](const juce::AudioBuffer<float>& out)
    {
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < out.getNumSamples(); ++i)
                maxError = juce::jmax(maxError, std::abs(out.getSample(ch, i) - gen(start + i, ch)));
        start += out.getNumSamples();
    });
    check(maxError == 0.0f, "audio passes through unchanged");
}

static void testNoAllocations()
{
    std::cout << "No allocations in processBlock" << std::endl;
    for (double sr : { 44100.0, 48000.0, 96000.0 })
    {
        for (int block : { 32, 441, 1024, 4096 })
        {
            Rig rig;
            rig.prepare(sr, block);
            rig.run(0.5, rig.sine(-12.0f, 1000.0)); // warm up

            allocationCount = 0;
            countAllocations = true;
            rig.run(1.0, rig.sine(-12.0f, 1000.0));
            rig.proc.setDevOscEnabled(true);
            rig.proc.resetLoudnessSession();
            rig.proc.resetHistogram();
            rig.run(0.5, rig.sine(-12.0f, 1000.0));
            rig.proc.setDevOscEnabled(false);
            countAllocations = false;

            check(allocationCount.load() == 0,
                  "processBlock allocated " + juce::String(allocationCount.load()) + " times at "
                      + juce::String(sr) + " Hz, block " + juce::String(block));
        }
    }
}

//==============================================================================
static void collectInteractive(juce::Component& root, juce::Array<juce::Component*>& out)
{
    for (auto* c : root.getChildren())
    {
        if (! c->isVisible())
            continue;
        if (dynamic_cast<juce::Button*>(c) != nullptr || dynamic_cast<juce::ComboBox*>(c) != nullptr
            || dynamic_cast<MeterModule*>(c) != nullptr)
            out.add(c);
        collectInteractive(*c, out);
    }
}

static void testEditor()
{
    std::cout << "Editor" << std::endl;

    auto procHolder = std::make_unique<FF360MeterProcessor>();
    auto& proc = *procHolder;
    proc.setRateAndBufferSizeDetails(48000.0, 512);
    proc.prepareToPlay(48000.0, 512);

    {
        std::unique_ptr<juce::AudioProcessorEditor> editorHolder(proc.createEditorIfNeeded());
        auto* editor = dynamic_cast<FF360MeterEditor*>(editorHolder.get());
        check(editor != nullptr, "editor is an FF360MeterEditor");
        if (editor == nullptr)
            return;

        // Every layout's controls, and every module, have a tooltip
        for (const auto& layout : ff360_labs::DashboardLayout::getFactoryPresets())
        {
            editor->loadLayout(layout);
            juce::Array<juce::Component*> interactive;
            collectInteractive(*editor, interactive);
            int missing = 0;
            for (auto* c : interactive)
                if (auto* tc = dynamic_cast<juce::TooltipClient*>(c); tc == nullptr || tc->getTooltip().isEmpty())
                {
                    ++missing;
                    std::cout << "    no tooltip: " << c->getName() << " (" << typeid(*c).name() << ")" << std::endl;
                }
            check(missing == 0, "every control has a tooltip in " + layout.name + " (" + juce::String(interactive.size()) + " checked)");
        }

        // Fonts resolve to the embedded brand faces
        // The embedded faces report family "Barlow" with the width in the style ("Condensed Medium")
        const auto ui = FF360LabsLookAndFeel::getUiFont(14.0f);
        check(ui.getTypefaceName() == "Barlow" && ui.getTypefaceStyle().contains("Condensed"),
              "UI text uses Barlow Condensed (" + ui.getTypefaceName() + " " + ui.getTypefaceStyle() + ")");
        check(FF360LabsLookAndFeel::getCustomFont(12.0f).getTypefaceName().contains("JetBrains Mono"),
              "readouts use JetBrains Mono (" + FF360LabsLookAndFeel::getCustomFont(12.0f).getTypefaceName() + ")");

        // Zoom scales the canvas and the window, keeping the logical layout
        editor->setSize(1200, 700);
        editor->setUiZoom(1.5f);
        check(editor->getWidth() == 1800 && editor->getHeight() == 1050, "150% zoom scales the window ("
              + juce::String(editor->getWidth()) + "x" + juce::String(editor->getHeight()) + ")");
        check(editor->getCanvas().getWidth() == 1200 && editor->getCanvas().getHeight() == 700, "the canvas keeps its logical size");
        check(editor->getCanvas().getTransform().mat00 == 1.5f, "the canvas is scaled 1.5x");
        auto* constrainer = editor->getConstrainer();
        check(constrainer != nullptr && constrainer->getMinimumWidth() == juce::roundToInt(FF360MeterEditor::minLogicalWidth * 1.5f),
              "the minimum size follows the zoom");

        // Layout changes are in the session without closing the editor
        editor->loadLayout(ff360_labs::DashboardLayout::getFactoryPresets()[2]); // Quick Check: 2 modules
        editor->getDashboard().getModules().getFirst()->onClose(editor->getDashboard().getModules().getFirst());
        const auto active = ff360_labs::DashboardLayout::fromValueTree(proc.apvts.state.getChildWithName("ActiveLayout"));
        check(active.moduleTypes.size() == 1, "removing a module updates the saved layout immediately");

        // Double-clicking the LUFS module resets the measurement
        editor->loadLayout(ff360_labs::DashboardLayout::getFactoryPresets()[2]);
        Rig rig; // reuse the signal helper on this processor
        juce::AudioBuffer<float> block(2, 512);
        juce::MidiBuffer midi;
        const auto gen = rig.sine(-20.0f, 1000.0);
        auto feed = [&](double seconds)
        {
            for (int b = 0; b < (int) (seconds * 48000.0 / 512); ++b)
            {
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < 512; ++i)
                        block.setSample(ch, i, gen(rig.sampleIndex + i, ch));
                rig.sampleIndex += 512;
                proc.processBlock(block, midi);
            }
        };
        feed(2.0);
        for (auto* m : editor->getDashboard().getModules())
            if (m->getModuleType() == MeterModuleType::LUFS)
                m->mouseDoubleClick(juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(), {}, {}, 1.0f,
                                                     0.0f, 0.0f, 0.0f, 0.0f, m, m, juce::Time::getCurrentTime(),
                                                     {}, juce::Time::getCurrentTime(), 2, false));
        feed(0.02);
        check(proc.lufsDSP.getIntegrated() <= -69.0f, "double-clicking the LUFS module resets Integrated");

        proc.editorBeingDeleted(editor);
    }

    // Zoom and size survive a session reload
    juce::MemoryBlock saved;
    proc.getStateInformation(saved);

    auto restoredHolder = std::make_unique<FF360MeterProcessor>();
    auto& restored = *restoredHolder;
    restored.setStateInformation(saved.getData(), (int) saved.getSize());
    std::unique_ptr<juce::AudioProcessorEditor> reopened(restored.createEditorIfNeeded());
    auto* editor = dynamic_cast<FF360MeterEditor*>(reopened.get());
    check(editor != nullptr && std::abs(editor->getUiZoom() - 1.5f) < 0.001f, "zoom is restored with the session");
    check(editor != nullptr && editor->getWidth() == 1800 && editor->getHeight() == 1050, "window size is restored with the session");
    if (editor != nullptr)
        restored.editorBeingDeleted(editor);
}

//==============================================================================
// Renders the editor, with signal through it, into a PNG at the given scale
static void writeScreenshot(const juce::File& file, float scale, const juce::String& layoutName)
{
    auto procHolder = std::make_unique<FF360MeterProcessor>();
    auto& proc = *procHolder;
    proc.setRateAndBufferSizeDetails(48000.0, 512);
    proc.prepareToPlay(48000.0, 512);

    std::unique_ptr<juce::AudioProcessorEditor> editorHolder(proc.createEditorIfNeeded());
    auto* editor = dynamic_cast<FF360MeterEditor*>(editorHolder.get());
    for (const auto& layout : ff360_labs::DashboardLayout::getFactoryPresets())
        if (layout.name == layoutName)
            editor->loadLayout(layout);
    editor->setSize(1280, 800);

    // A music-ish signal: two detuned tones, a slow swell and some decorrelation
    juce::AudioBuffer<float> block(2, 512);
    juce::MidiBuffer midi;
    long long n = 0;
    for (int b = 0; b < 1200; ++b)
    {
        for (int i = 0; i < 512; ++i, ++n)
        {
            const double t = (double) n / 48000.0;
            const double swell = 0.55 + 0.45 * std::sin(t * 0.9);
            const double base = 0.25 * std::sin(juce::MathConstants<double>::twoPi * 110.0 * t)
                              + 0.12 * std::sin(juce::MathConstants<double>::twoPi * 1760.0 * t);
            block.setSample(0, i, (float) (swell * (base + 0.08 * std::sin(juce::MathConstants<double>::twoPi * 5200.0 * t))));
            block.setSample(1, i, (float) (swell * (base + 0.08 * std::sin(juce::MathConstants<double>::twoPi * 3300.0 * t + 1.0))));
        }
        proc.processBlock(block, midi);

        // Drain the queues as the UI would, a frame every ~10 ms of audio
        for (auto* m : editor->getDashboard().getModules())
                if (auto* timer = dynamic_cast<juce::Timer*>(m))
                    timer->timerCallback();
    }

    auto image = editor->createComponentSnapshot(editor->getLocalBounds(), true, scale);
    file.deleteFile();
    juce::FileOutputStream out(file);
    juce::PNGImageFormat().writeImageToStream(image, out);
    proc.editorBeingDeleted(editor);
}

// Assertion failures and leak reports (JUCE_LOG_ASSERTIONS) go to stdout and count as failures
struct StdoutLogger : public juce::Logger
{
    void logMessage(const juce::String& message) override
    {
        // C stdio: std::cout may already be gone when leak reports arrive
        std::fprintf(stdout, "  LOG: %s\n", message.toRawUTF8());
        std::fflush(stdout);
        if (message.containsIgnoreCase("leak"))
            std::_Exit(2); // reported after main returned: fail the process directly
        if (message.contains("Assertion"))
            ++failures;
    }
};

// Never destroyed or detached: leak reports arrive during static destruction, after main returns
// (and ~Logger asserts if it is still the current logger)
static StdoutLogger& stdoutLogger = *new StdoutLogger();

int main(int argc, char** argv)
{
    juce::Logger::setCurrentLogger(&stdoutLogger);

    juce::ScopedJuceInitialiser_GUI juceInit;

    // --screenshot <file.png> [scale] [layout]
    if (argc >= 3 && juce::String(argv[1]) == "--screenshot")
    {
        writeScreenshot(juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]),
                        argc >= 4 ? juce::String(argv[3]).getFloatValue() : 1.0f,
                        argc >= 5 ? juce::String(argv[4]) : juce::String("Full Suite"));
        return 0;
    }

    std::cout << "FF360Meter - processor tests" << std::endl;

    testSineCalibration();
    testEbuReference();
    testMonoLoudness();
    testNegativePeaks();
    testSpectrumCalibration();
    testFftResolutionSwitch();
    testCorrelation();
    testResetsAndSessionMaxima();
    testDevOsc();
    testPassThrough();
    testNoAllocations();
    testEditor();

    std::cout << std::endl << (checks - failures) << "/" << checks << " checks passed" << std::endl;
    return failures == 0 ? 0 : 1;
}
