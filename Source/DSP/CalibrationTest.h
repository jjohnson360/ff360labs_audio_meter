#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

#include "PeakRmsDSP.h"
#include "VuDSP.h"
#include "LufsDSP.h"

// =============================================================================
// CalibrationTestRunner  (Phase 10.7)
//
// Written source of truth — expected readings for reference signals:
//
//  Signal                  | Meter         | Expected reading
//  ------------------------|---------------|----------------------------------
//  0 dBFS full-scale sine  | Peak          |  0.00 dBFS
//  0 dBFS full-scale sine  | RMS           | -3.01 dBFS
//  0 dBFS full-scale sine  | VU (ref=-18)  | +14.99 VU (clamps to +3 display)
//  -20 dBFS sine           | Peak          | -20.00 dBFS
//  -20 dBFS sine           | RMS           | -23.01 dBFS
//  -20 dBFS sine           | VU (ref=-18)  | -5.01 VU
//  -18 dBFS sine           | VU (ref=-18)  |  0.00 VU  (calibration point)
//  -14 dBFS sine           | VU (ref=-14)  |  0.00 VU  (streaming ref point)
//
// Acceptable tolerance for a passing test: +/- 0.25 dB
// =============================================================================

namespace ff360_labs
{

struct CalibrationResult
{
    float peakDb    = 0.0f;
    float rmsDb     = 0.0f;
    float vuDb      = 0.0f;

    float expectedPeakDb = 0.0f;
    float expectedRmsDb  = 0.0f;
    float expectedVuDb   = 0.0f;

    static constexpr float toleranceDb = 0.25f;

    bool peakPass() const { return std::abs(peakDb - expectedPeakDb) <= toleranceDb; }
    bool rmsPass()  const { return std::abs(rmsDb  - expectedRmsDb)  <= toleranceDb; }
    bool vuPass()   const { return std::abs(vuDb   - expectedVuDb)   <= toleranceDb; }

    juce::String summary() const
    {
        return juce::String("Peak: ") + juce::String(peakDb,2) + " dBFS (exp " + juce::String(expectedPeakDb,2) + ") " + (peakPass() ? "PASS" : "FAIL") + "\n"
             + juce::String("RMS:  ") + juce::String(rmsDb,2)  + " dBFS (exp " + juce::String(expectedRmsDb,2)  + ") " + (rmsPass()  ? "PASS" : "FAIL") + "\n"
             + juce::String("VU:   ") + juce::String(vuDb,2)   + " VU   (exp " + juce::String(expectedVuDb,2)   + ") " + (vuPass()   ? "PASS" : "FAIL");
    }
};

class CalibrationTestRunner
{
public:
    // Generates a stereo buffer filled with a calibrated 1 kHz sine at sinePeakDbfs.
    static juce::AudioBuffer<float> generateSineBlock(float sinePeakDbfs,
                                                       double sampleRate,
                                                       float durationSeconds = 1.2f)
    {
        float peakLinear = std::pow(10.0f, sinePeakDbfs / 20.0f);
        int numSamples = static_cast<int>(sampleRate * durationSeconds);
        juce::AudioBuffer<float> buffer(2, numSamples);

        float freq = 1000.0f;
        float phaseInc = juce::MathConstants<float>::twoPi * freq / static_cast<float>(sampleRate);
        float phase = 0.0f;
        auto* L = buffer.getWritePointer(0);
        auto* R = buffer.getWritePointer(1);

        for (int i = 0; i < numSamples; ++i)
        {
            float s = peakLinear * std::sin(phase);
            L[i] = s;
            R[i] = s;
            phase += phaseInc;
            if (phase > juce::MathConstants<float>::twoPi)
                phase -= juce::MathConstants<float>::twoPi;
        }
        return buffer;
    }

    // Runs PeakRmsDSP and VuDSP against a sine at sinePeakDbfs, returns measured vs expected.
    static CalibrationResult runSineTest(float sinePeakDbfs,
                                          float vuRefLevelDb,
                                          double sampleRate)
    {
        CalibrationResult result;
        result.expectedPeakDb = sinePeakDbfs;
        result.expectedRmsDb  = sinePeakDbfs - 3.01f;
        result.expectedVuDb   = (sinePeakDbfs - 3.01f) - vuRefLevelDb;

        auto buffer = generateSineBlock(sinePeakDbfs, sampleRate);

        // PeakRmsDSP
        {
            PeakRmsDSP peakDsp;
            peakDsp.prepare(sampleRate, buffer.getNumSamples());
            auto data = peakDsp.processBlock(buffer);
            result.peakDb = data.peakL;
            result.rmsDb  = data.rmsL;
        }

        // VuDSP — process in 512-sample blocks so IIR ballistics converge
        {
            VuDSP vuDsp;
            vuDsp.prepare(sampleRate, 512);
            vuDsp.setReferenceLevelDb(vuRefLevelDb);

            constexpr int kBlock = 512;
            VuMeterData vuData;
            for (int offset = 0; offset + kBlock <= buffer.getNumSamples(); offset += kBlock)
            {
                juce::AudioBuffer<float> blk(2, kBlock);
                blk.copyFrom(0, 0, buffer, 0, offset, kBlock);
                blk.copyFrom(1, 0, buffer, 1, offset, kBlock);
                vuData = vuDsp.processBlock(blk);
            }
            result.vuDb = vuData.vuL;
        }

        return result;
    }

    // Runs the standard three-point suite and logs to DBG output.
    // Gate this call with #if JUCE_DEBUG to keep it out of release builds.
    static void runAndLogFullSuite(double sampleRate)
    {
        DBG("=== ff360_labs Calibration Audit (Phase 10.7) ===");

        auto r1 = runSineTest(-20.0f, -18.0f, sampleRate);
        DBG("-- -20 dBFS sine, VU ref=-18 dBFS --");
        DBG(r1.summary());

        auto r2 = runSineTest(-18.0f, -18.0f, sampleRate);
        DBG("-- -18 dBFS sine, VU ref=-18 dBFS (0 VU calibration point) --");
        DBG(r2.summary());

        auto r3 = runSineTest(0.0f, -18.0f, sampleRate);
        DBG("-- 0 dBFS full-scale sine, VU ref=-18 dBFS --");
        DBG(r3.summary());

        DBG("=================================================");
    }

private:
    CalibrationTestRunner() = delete;
};

} // namespace ff360_labs
