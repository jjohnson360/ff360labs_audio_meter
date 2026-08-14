#pragma once
#include <juce_core/juce_core.h>
#include <vector>

namespace ff360_labs
{
    struct LoudnessTarget
    {
        juce::String name;
        float targetLufs;      // Target Integrated LUFS (e.g. -14.0f)
        float tolerance;       // +/- tolerance window in LU (e.g. 1.0f)
        float maxTruePeakDb;   // Maximum allowable True Peak in dBTP (e.g. -1.0f)

        static const std::vector<LoudnessTarget>& getBuiltinPresets()
        {
            static const std::vector<LoudnessTarget> presets = {
                { "Spotify (-14 LUFS, -1.0 dBTP)",       -14.0f, 1.0f, -1.0f },
                { "YouTube (-14 LUFS, -1.0 dBTP)",       -14.0f, 1.0f, -1.0f },
                { "Apple Music (-16 LUFS, -1.0 dBTP)",   -16.0f, 1.0f, -1.0f },
                { "Netflix (-27 LUFS, -2.0 dBTP)",       -27.0f, 1.0f, -2.0f },
                { "EBU R128 (-23 LUFS, -1.0 dBTP)",      -23.0f, 0.5f, -1.0f },
                { "Club / Master (-9 LUFS, -0.3 dBTP)",  -9.0f,  1.5f, -0.3f },
                { "AES Streaming (-16 LUFS, -1.0 dBTP)", -16.0f, 1.0f, -1.0f },
                { "Custom (-18 LUFS, -1.0 dBTP)",        -18.0f, 1.0f, -1.0f }
            };
            return presets;
        }

        static juce::StringArray getPresetNames()
        {
            juce::StringArray names;
            for (const auto& p : getBuiltinPresets())
                names.add(p.name);
            return names;
        }

        static LoudnessTarget getPresetByIndex(int index)
        {
            const auto& presets = getBuiltinPresets();
            if (index >= 0 && index < (int)presets.size())
                return presets[(size_t)index];
            return presets[0];
        }
    };
}
