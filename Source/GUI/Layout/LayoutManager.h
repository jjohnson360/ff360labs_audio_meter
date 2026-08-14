#pragma once
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include "../Common/MeterModule.h"
#include "MeterDashboard.h"
#include <vector>

namespace ff360_labs
{
    struct DashboardLayout
    {
        juce::String name;
        LayoutMode mode { LayoutMode::Grid };
        std::vector<MeterModuleType> moduleTypes;

        static juce::String moduleTypeToString (MeterModuleType type)
        {
            switch (type)
            {
                case MeterModuleType::PeakRms:    return "PeakRms";
                case MeterModuleType::VU:         return "VU";
                case MeterModuleType::LUFS:       return "LUFS";
                case MeterModuleType::PhaseScope: return "PhaseScope";
                case MeterModuleType::Spectrum:   return "Spectrum";
                case MeterModuleType::Histogram:  return "Histogram";
                default:                          return "Unknown";
            }
        }

        static MeterModuleType stringToModuleType (const juce::String& str)
        {
            if (str == "PeakRms")    return MeterModuleType::PeakRms;
            if (str == "VU")         return MeterModuleType::VU;
            if (str == "LUFS")       return MeterModuleType::LUFS;
            if (str == "PhaseScope") return MeterModuleType::PhaseScope;
            if (str == "Spectrum")   return MeterModuleType::Spectrum;
            if (str == "Histogram")  return MeterModuleType::Histogram;
            return MeterModuleType::Unknown;
        }

        static const std::vector<DashboardLayout>& getFactoryPresets()
        {
            static const std::vector<DashboardLayout> factory = {
                { "Mastering", LayoutMode::Grid, { MeterModuleType::PeakRms, MeterModuleType::LUFS, MeterModuleType::Spectrum, MeterModuleType::PhaseScope } },
                { "Broadcast QC", LayoutMode::Grid, { MeterModuleType::LUFS, MeterModuleType::Histogram, MeterModuleType::PeakRms, MeterModuleType::VU } },
                { "Quick Check", LayoutMode::Grid, { MeterModuleType::PeakRms, MeterModuleType::LUFS } },
                { "Full Suite", LayoutMode::Grid, { MeterModuleType::PeakRms, MeterModuleType::VU, MeterModuleType::LUFS, MeterModuleType::Spectrum, MeterModuleType::Histogram, MeterModuleType::PhaseScope } }
            };
            return factory;
        }

        juce::ValueTree toValueTree(const juce::Identifier& typeIdentifier = "DashboardLayout") const
        {
            juce::ValueTree vt (typeIdentifier);
            vt.setProperty ("name", name, nullptr);
            vt.setProperty ("mode", mode == LayoutMode::Maximized ? "Maximized" : "Grid", nullptr);

            for (auto type : moduleTypes)
            {
                juce::ValueTree child ("Module");
                child.setProperty ("type", moduleTypeToString (type), nullptr);
                vt.addChild (child, -1, nullptr);
            }
            return vt;
        }

        static DashboardLayout fromValueTree (const juce::ValueTree& vt)
        {
            DashboardLayout layout;
            layout.name = vt.getProperty ("name", "Custom");
            juce::String modeStr = vt.getProperty ("mode", "Grid");
            layout.mode = (modeStr == "Maximized") ? LayoutMode::Maximized : LayoutMode::Grid;

            for (int i = 0; i < vt.getNumChildren(); ++i)
            {
                auto child = vt.getChild (i);
                if (child.hasType ("Module"))
                {
                    layout.moduleTypes.push_back (stringToModuleType (child.getProperty ("type", "")));
                }
            }
            return layout;
        }
    };
}
