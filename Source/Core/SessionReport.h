#pragma once
#include <juce_core/juce_core.h>
#include "LoudnessTarget.h"

namespace ff360_labs
{
    struct SessionReportData
    {
        juce::String sessionName { "Audio Mastering Session" };
        juce::String timestamp;
        LoudnessTarget target;

        float integratedLufs { -70.0f };
        float lra { 0.0f };
        float shortTermMax { -70.0f };
        float momentaryMax { -70.0f };
        float peakL { -60.0f };
        float peakR { -60.0f };

        bool isCompliant { false };
        juce::String complianceStatus { "PENDING" };
        float targetDelta { 0.0f };

        static SessionReportData collect (const LoudnessTarget& target,
                                          float integrated, float lraVal,
                                          float shortTerm, float momentary,
                                          float peakLeft, float peakRight);

        juce::String toCsv() const;
        juce::String toBrandedHtml() const;

        bool exportCsv (const juce::File& file) const;
        bool exportHtml (const juce::File& file) const;
    };
}
