#include "SessionReport.h"

namespace ff360_labs
{
    SessionReportData SessionReportData::collect (const LoudnessTarget& targetProfile,
                                                  float integrated, float lraVal,
                                                  float shortTerm, float momentary,
                                                  float peakLeft, float peakRight)
    {
        SessionReportData d;
        d.timestamp = juce::Time::getCurrentTime().toString(true, true);
        d.target = targetProfile;
        d.integratedLufs = integrated;
        d.lra = lraVal;
        d.shortTermMax = shortTerm;
        d.momentaryMax = momentary;
        d.peakL = peakLeft;
        d.peakR = peakRight;

        d.targetDelta = (integrated <= -69.0f) ? 0.0f : (integrated - targetProfile.targetLufs);

        if (integrated <= -69.0f)
        {
            d.isCompliant = false;
            d.complianceStatus = "NO SIGNAL / SILENCE";
        }
        else if (std::abs(d.targetDelta) <= targetProfile.tolerance)
        {
            d.isCompliant = true;
            d.complianceStatus = "PASS - COMPLIANT";
        }
        else if (d.targetDelta > targetProfile.tolerance)
        {
            d.isCompliant = false;
            d.complianceStatus = "FAIL - EXCEEDS TARGET";
        }
        else
        {
            d.isCompliant = false;
            d.complianceStatus = "FAIL - BELOW TARGET";
        }

        return d;
    }

    juce::String SessionReportData::toCsv() const
    {
        juce::String csv;
        csv << "ff360_labs Modular Audio Meter - Session Loudness & Peak Report\n";
        csv << "Report Timestamp," << timestamp << "\n";
        csv << "Target Profile," << target.name << "\n";
        csv << "Target LUFS-I (LUFS)," << juce::String(target.targetLufs, 1) << "\n";
        csv << "Tolerance (+/- LU)," << juce::String(target.tolerance, 1) << "\n";
        csv << "Max True-Peak Ceiling (dBTP)," << juce::String(target.maxTruePeakDb, 1) << "\n";
        csv << "\n";
        csv << "Metric,Measured Value,Unit,Target Delta,Status\n";
        csv << "Integrated Loudness (LUFS-I)," << (integratedLufs <= -69.0f ? "-inf" : juce::String(integratedLufs, 2)) << ",LUFS,"
            << (integratedLufs <= -69.0f ? "N/A" : (targetDelta > 0 ? "+" : "") + juce::String(targetDelta, 2) + " LU") << ","
            << complianceStatus << "\n";
        csv << "Loudness Range (LRA)," << juce::String(lra, 2) << ",LU,N/A," << (lra <= 14.0f ? "OK" : "WIDE DYNAMIC RANGE") << "\n";
        csv << "Short-Term Max," << (shortTermMax <= -69.0f ? "-inf" : juce::String(shortTermMax, 2)) << ",LUFS,N/A,INFO\n";
        csv << "Momentary Max," << (momentaryMax <= -69.0f ? "-inf" : juce::String(momentaryMax, 2)) << ",LUFS,N/A,INFO\n";
        csv << "Peak Left," << (peakL <= -59.0f ? "-inf" : juce::String(peakL, 2)) << ",dBFS,N/A," << (peakL > -0.1f ? "CLIPPING" : "OK") << "\n";
        csv << "Peak Right," << (peakR <= -59.0f ? "-inf" : juce::String(peakR, 2)) << ",dBFS,N/A," << (peakR > -0.1f ? "CLIPPING" : "OK") << "\n";
        return csv;
    }

    juce::String SessionReportData::toBrandedHtml() const
    {
        juce::String badgeColor = isCompliant ? "#c9a15a" : "#e8654a";
        juce::String statusText = complianceStatus;

        juce::String html;
        html << "<!DOCTYPE html>\n<html>\n<head>\n";
        html << "<meta charset='UTF-8'>\n";
        html << "<title>ff360_labs Session Report - " << timestamp << "</title>\n";
        html << "<style>\n";
        html << "  body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; background: #0a0a0b; color: #edeae3; margin: 0; padding: 40px; }\n";
        html << "  .container { max-width: 820px; margin: 0 auto; background: #17171a; border: 1px solid rgba(201, 161, 90, 0.25); border-radius: 12px; padding: 36px; box-shadow: 0 12px 36px rgba(0,0,0,0.6); }\n";
        html << "  .header { display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid rgba(201, 161, 90, 0.2); padding-bottom: 20px; margin-bottom: 24px; }\n";
        html << "  .brand { font-size: 24px; font-weight: 700; color: #c9a15a; letter-spacing: 0.5px; }\n";
        html << "  .brand span { color: #8a8780; font-size: 16px; font-weight: 400; }\n";
        html << "  .timestamp { color: #8a8780; font-size: 13px; font-family: monospace; }\n";
        html << "  .badge { display: inline-block; padding: 6px 14px; border-radius: 4px; font-size: 13px; font-weight: 700; letter-spacing: 0.5px; background: " << badgeColor << "22; color: " << badgeColor << "; border: 1px solid " << badgeColor << "; }\n";
        html << "  .grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 16px; margin-bottom: 28px; }\n";
        html << "  .card { background: rgba(10, 10, 11, 0.6); border: 1px solid rgba(201, 161, 90, 0.15); border-radius: 8px; padding: 18px; text-align: center; }\n";
        html << "  .card .label { font-size: 11px; color: #8a8780; font-weight: 600; text-transform: uppercase; margin-bottom: 6px; letter-spacing: 0.5px; }\n";
        html << "  .card .val { font-size: 26px; font-weight: 700; color: #edeae3; font-family: monospace; }\n";
        html << "  .card .unit { font-size: 11px; color: #c9a15a; margin-top: 2px; }\n";
        html << "  table { width: 100%; border-collapse: collapse; margin-top: 16px; font-size: 14px; }\n";
        html << "  th { text-align: left; padding: 10px 12px; color: #8a8780; border-bottom: 1px solid rgba(201, 161, 90, 0.2); font-size: 11px; text-transform: uppercase; }\n";
        html << "  td { padding: 12px; border-bottom: 1px solid rgba(255, 255, 255, 0.05); }\n";
        html << "  .target-box { background: rgba(201, 161, 90, 0.08); border-left: 3px solid #c9a15a; padding: 14px 18px; border-radius: 4px; margin-bottom: 24px; font-size: 14px; }\n";
        html << "  .footer { margin-top: 32px; text-align: center; font-size: 11px; color: #8a8780; border-top: 1px solid rgba(255,255,255,0.05); padding-top: 16px; }\n";
        html << "  @media print { body { background: #fff; color: #111; padding: 0; } .container { box-shadow: none; border: 1px solid #ccc; background: #fafafa; } .card { background: #fff; border: 1px solid #ddd; } .card .val { color: #111; } }\n";
        html << "</style>\n</head>\n<body>\n";
        html << "<div class='container'>\n";
        html << "  <div class='header'>\n";
        html << "    <div>\n";
        html << "      <div class='brand'>ff360_labs <span>// MASTERING QUALITY REPORT</span></div>\n";
        html << "      <div class='timestamp'>Generated: " << timestamp << "</div>\n";
        html << "    </div>\n";
        html << "    <div><span class='badge'>" << statusText << "</span></div>\n";
        html << "  </div>\n";
        html << "  <div class='target-box'>\n";
        html << "    <strong>Target Profile:</strong> " << target.name << " &bull; <strong>Target:</strong> " << juce::String(target.targetLufs, 1) << " LUFS-I (&plusmn;" << juce::String(target.tolerance, 1) << " LU) &bull; <strong>Peak Ceiling:</strong> " << juce::String(target.maxTruePeakDb, 1) << " dBTP\n";
        html << "  </div>\n";
        html << "  <div class='grid'>\n";
        html << "    <div class='card'><div class='label'>Integrated LUFS</div><div class='val' style='color:" << (isCompliant ? "#c9a15a" : "#e8654a") << "'>" << (integratedLufs <= -69.0f ? "-inf" : juce::String(integratedLufs, 1)) << "</div><div class='unit'>LUFS</div></div>\n";
        html << "    <div class='card'><div class='label'>Loudness Range</div><div class='val'>" << juce::String(lra, 1) << "</div><div class='unit'>LU</div></div>\n";
        html << "    <div class='card'><div class='label'>Short-Term Max</div><div class='val'>" << (shortTermMax <= -69.0f ? "-inf" : juce::String(shortTermMax, 1)) << "</div><div class='unit'>LUFS</div></div>\n";
        html << "  </div>\n";
        html << "  <table>\n";
        html << "    <thead><tr><th>Metric</th><th>Measured</th><th>Target / Reference</th><th>Delta</th><th>Result</th></tr></thead>\n";
        html << "    <tbody>\n";
        html << "      <tr><td><strong>Integrated Loudness (LUFS-I)</strong></td><td>" << (integratedLufs <= -69.0f ? "-inf" : juce::String(integratedLufs, 2)) << " LUFS</td><td>" << juce::String(target.targetLufs, 1) << " LUFS (&plusmn;" << juce::String(target.tolerance, 1) << ")</td><td>" << (integratedLufs <= -69.0f ? "N/A" : (targetDelta > 0 ? "+" : "") + juce::String(targetDelta, 2) + " LU") << "</td><td><strong style='color:" << badgeColor << "'>" << complianceStatus << "</strong></td></tr>\n";
        html << "      <tr><td><strong>Loudness Range (LRA)</strong></td><td>" << juce::String(lra, 2) << " LU</td><td>Dynamic Spread</td><td>-</td><td>" << (lra <= 14.0f ? "Optimal" : "Wide") << "</td></tr>\n";
        html << "      <tr><td><strong>Short-Term Max (LUFS-S)</strong></td><td>" << (shortTermMax <= -69.0f ? "-inf" : juce::String(shortTermMax, 2)) << " LUFS</td><td>3-Second Sliding Window</td><td>-</td><td>Recorded</td></tr>\n";
        html << "      <tr><td><strong>Momentary Max (LUFS-M)</strong></td><td>" << (momentaryMax <= -69.0f ? "-inf" : juce::String(momentaryMax, 2)) << " LUFS</td><td>400ms Sliding Window</td><td>-</td><td>Recorded</td></tr>\n";
        html << "      <tr><td><strong>Sample Peak Left (Peak L)</strong></td><td>" << (peakL <= -59.0f ? "-inf" : juce::String(peakL, 2)) << " dBFS</td><td>< " << juce::String(target.maxTruePeakDb, 1) << " dBTP</td><td>-</td><td>" << (peakL > target.maxTruePeakDb ? "<span style='color:#e8654a'>OVER</span>" : "PASS") << "</td></tr>\n";
        html << "      <tr><td><strong>Sample Peak Right (Peak R)</strong></td><td>" << (peakR <= -59.0f ? "-inf" : juce::String(peakR, 2)) << " dBFS</td><td>< " << juce::String(target.maxTruePeakDb, 1) << " dBTP</td><td>-</td><td>" << (peakR > target.maxTruePeakDb ? "<span style='color:#e8654a'>OVER</span>" : "PASS") << "</td></tr>\n";
        html << "    </tbody>\n";
        html << "  </table>\n";
        html << "  <div class='footer'>ff360_labs Modular Audio Metering Suite &bull; Certified Loudness Compliance Deliverable</div>\n";
        html << "</div>\n</body>\n</html>\n";
        return html;
    }

    bool SessionReportData::exportCsv (const juce::File& file) const
    {
        return file.replaceWithText (toCsv());
    }

    bool SessionReportData::exportHtml (const juce::File& file) const
    {
        return file.replaceWithText (toBrandedHtml());
    }
}
