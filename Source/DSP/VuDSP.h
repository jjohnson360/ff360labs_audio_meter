#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

struct VuMeterData
{
    float vuL = -60.0f;
    float vuR = -60.0f;
    float rawDbfslL = -60.0f; // Unscaled raw RMS dBFS for dev debug
    float rawDbfslR = -60.0f;
};

class VuDSP
{
public:
    static constexpr float DEFAULT_VU_ATTACK_TIME_SEC  = 0.12f; // ~120ms fast rise time
    static constexpr float DEFAULT_VU_RELEASE_TIME_SEC = 0.35f; // ~350ms smooth decay time

    struct CalibrationPreset
    {
        const char* name;
        float refDb;
    };

    static inline const std::vector<CalibrationPreset>& getCalibrationPresets()
    {
        static const std::vector<CalibrationPreset> presets = {
            { "-18 dBFS (Broadcast / SMPTE)", -18.0f },
            { "-20 dBFS (EBU Standard)",     -20.0f },
            { "-14 dBFS (Streaming / Master)", -14.0f },
            { "-12 dBFS (Hot Masters)",       -12.0f },
            { "-10 dBFS (Club / High Level)", -10.0f }
        };
        return presets;
    }

    VuDSP();

    void prepare(double sampleRate, int samplesPerBlock);
    
    // Process block and return current VU values (in dB relative to 0 VU reference)
    VuMeterData processBlock(const juce::AudioBuffer<float>& buffer);

    // Set the reference level where 0 VU corresponds to (default -18 dBFS)
    void setReferenceLevelDb(float refLevelDb);
    float getReferenceLevelDb() const { return referenceLevelDb; }

    // Expose adjustable ballistics
    void setBallistics(float attackSec, float releaseSec);

    static float gainToVuDb(float linearGain);

private:
    double currentSampleRate = 48000.0;
    float referenceLevelDb = -18.0f;
    
    float attackTimeSec = DEFAULT_VU_ATTACK_TIME_SEC;
    float releaseTimeSec = DEFAULT_VU_RELEASE_TIME_SEC;
    float alphaAttack = 0.0f;
    float alphaRelease = 0.0f;

    float statePowerL = 0.0f;
    float statePowerR = 0.0f;

    void updateCoefficients();
};

