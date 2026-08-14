#include "PeakRmsDSP.h"
#include "../Core/Constants.h"
#include <cmath>

PeakRmsDSP::PeakRmsDSP() { updateCoefficients(); }

void PeakRmsDSP::prepare(double sampleRate, int /*samplesPerBlock*/) {
  currentSampleRate = sampleRate;
  updateCoefficients();
}

void PeakRmsDSP::updateCoefficients() {
  // Simple 1-pole IIR coefficients for UI ballistics
  // Assuming UI frame rate / block rate is around 30-100Hz
  // Note: These coefficients are applied per UI frame, not per sample!

  // Attack: 10ms
  attackCoef = std::exp(-1.0f / (0.010f * 60.0f)); // Assuming ~60fps UI updates

  // Release: 300ms
  releaseCoef = std::exp(-1.0f / (0.300f * 60.0f));
}

float PeakRmsDSP::gainToDb(float linearGain) {
  if (linearGain <= 0.0f)
    return ff360_labs::METER_FLOOR_DB;

  float db = juce::Decibels::gainToDecibels(linearGain);
  return juce::jlimit(ff360_labs::METER_FLOOR_DB, ff360_labs::METER_CEILING_DB,
                      db);
}

MeterData PeakRmsDSP::processBlock(const juce::AudioBuffer<float> &buffer) {
  MeterData data;

  int numChannels = buffer.getNumChannels();
  int numSamples = buffer.getNumSamples();

  if (numChannels > 0 && numSamples > 0) {
    auto *leftData = buffer.getReadPointer(0);
    data.peakL = gainToDb(
        juce::FloatVectorOperations::findMaximum(leftData, numSamples));
    // NOTE: getRMSLevel() already returns the root-mean-square value (sqrt of
    // mean-square). Do NOT wrap in std::sqrt() again — that would compute
    // RMS^0.5 and read hot.
    data.rmsL = gainToDb(buffer.getRMSLevel(0, 0, numSamples));

    if (numChannels > 1) {
      auto *rightData = buffer.getReadPointer(1);
      data.peakR = gainToDb(
          juce::FloatVectorOperations::findMaximum(rightData, numSamples));
      data.rmsR = gainToDb(buffer.getRMSLevel(1, 0, numSamples));
    } else {
      // Mono
      data.peakR = data.peakL;
      data.rmsR = data.rmsL;
    }
  }

  return data;
}

void PeakRmsDSP::applyBallistics(MeterData &current, const MeterData &target) {
  // Apply smoothing per channel / metric
  auto smooth = [this](float &currentVal, float targetVal) {
    if (targetVal > currentVal) {
      currentVal = targetVal + attackCoef * (currentVal - targetVal);
    } else {
      currentVal = targetVal + releaseCoef * (currentVal - targetVal);
    }
  };

  smooth(current.peakL, target.peakL);
  smooth(current.peakR, target.peakR);
  smooth(current.rmsL, target.rmsL);
  smooth(current.rmsR, target.rmsR);
}
