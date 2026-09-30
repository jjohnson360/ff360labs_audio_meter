#include "PluginProcessor.h"
#include "../GUI/PluginEditor.h"

FF360MeterProcessor::FF360MeterProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    vuRefLevelParam = apvts.getRawParameterValue("vuRefLevel");
}

FF360MeterProcessor::~FF360MeterProcessor()
{
}

const juce::String FF360MeterProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FF360MeterProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FF360MeterProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FF360MeterProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FF360MeterProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FF360MeterProcessor::getNumPrograms()
{
    return 1;
}

int FF360MeterProcessor::getCurrentProgram()
{
    return 0;
}

void FF360MeterProcessor::setCurrentProgram (int index)
{
}

const juce::String FF360MeterProcessor::getProgramName (int index)
{
    return {};
}

void FF360MeterProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void FF360MeterProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    peakRmsDSP.prepare(sampleRate, samplesPerBlock);
    vuDSP.prepare(sampleRate, samplesPerBlock);
    lufsDSP.prepare(sampleRate, samplesPerBlock);
    phaseScopeDSP.prepare(sampleRate, samplesPerBlock);
    spectrumDSP.prepare(sampleRate, samplesPerBlock);
    histogramDSP.prepare(sampleRate, samplesPerBlock);
}

void FF360MeterProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool FF360MeterProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void FF360MeterProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Update VU Reference Level from APVTS
    if (vuRefLevelParam != nullptr)
    {
        int idx = (int) vuRefLevelParam->load();
        const auto& presets = VuDSP::getCalibrationPresets();
        if (idx >= 0 && idx < (int)presets.size())
            vuDSP.setReferenceLevelDb(presets[(size_t)idx].refDb);
    }

    // Phase 11: DEV OSC Internal Reference Tone Generator (1 kHz @ -18 dBFS)
    if (devOscEnabled.load())
    {
        double sr = getSampleRate();
        if (sr <= 0.0) sr = 48000.0;
        double phaseInc = juce::MathConstants<double>::twoPi * 1000.0 / sr;
        constexpr float oscAmp = 0.12589254f; // -18 dBFS
        int numSamples = buffer.getNumSamples();
        int numChannels = buffer.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            float s = static_cast<float>(std::sin(oscPhase) * oscAmp);
            oscPhase += phaseInc;
            if (oscPhase >= juce::MathConstants<double>::twoPi)
                oscPhase -= juce::MathConstants<double>::twoPi;

            for (int ch = 0; ch < numChannels; ++ch)
                buffer.setSample(ch, i, s);
        }
    }

    // Only the main input bus is metered
    const int numMeteredChannels = juce::jmin (buffer.getNumChannels(), juce::jmax (1, totalNumInputChannels), 2);
    if (numMeteredChannels == 0 || buffer.getNumSamples() == 0)
        return;

    // Refers to the host's channel data; no allocation for 1-2 channels
    juce::AudioBuffer<float> metered (buffer.getArrayOfWritePointers(), numMeteredChannels, buffer.getNumSamples());

    // Input Signal & Device Activity Monitoring
    bool isOscOn = devOscEnabled.load();
    bool hasInputs = isOscOn || (totalNumInputChannels > 0 && buffer.getNumSamples() > 0);
    isInputConnected.store(hasInputs);

    float maxMag = hasInputs ? metered.getMagnitude(0, metered.getNumSamples()) : 0.0f;
    float peakDb = (maxMag > 1e-5f) ? (20.0f * std::log10(maxMag)) : -100.0f;
    currentPeakLevelDb.store(peakDb);
    isAudioSilent.store(!isOscOn && (maxMag < 0.0001f)); // Lower than ~-80 dBFS considered idle silence

    if (sessionPeakResetRequested.exchange(false))
        for (auto& p : sessionPeakDb)
            p.store(-100.0f);

    for (int ch = 0; ch < 2; ++ch)
    {
        const int src = juce::jmin (ch, numMeteredChannels - 1);
        const float mag = metered.getMagnitude (src, 0, metered.getNumSamples());
        const float db = mag > 1e-5f ? 20.0f * std::log10 (mag) : -100.0f;
        if (db > sessionPeakDb[(size_t) ch].load())
            sessionPeakDb[(size_t) ch].store (db);
    }

    // Calculate Peak and RMS for this block
    MeterData blockData = peakRmsDSP.processBlock(metered);
    
    // Calculate VU for this block
    VuMeterData vuData = vuDSP.processBlock(metered);
    
    // Calculate LUFS for this block
    LufsMeterData lufsData = lufsDSP.processBlock(metered);
    
    // Calculate Phase Scope for this block
    phaseScopeDSP.processBlock(metered, phaseScratch);
    
    // Calculate Spectrum for this block
    bool newSpec = spectrumDSP.processBlock(metered, spectrumScratch);
    
    if (triggerHistogramReset.exchange(false))
    {
        histogramDSP.reset();
    }
    
    // Calculate Histogram for this block using Short-term LUFS
    bool newHist = histogramDSP.processBlock(metered, lufsData.shortTerm, histogramScratch);
    
    // Push the struct safely to the GUI thread
    meterFifo.push(blockData);
    vuFifo.push(vuData);
    lufsFifo.push(lufsData);
    phaseScopeFifo.push(phaseScratch);
    if (newSpec) spectrumFifo.push(spectrumScratch);
    if (newHist) histogramFifo.push(histogramScratch);
}

void FF360MeterProcessor::resetLoudnessSession()
{
    lufsDSP.requestReset();
    triggerHistogramReset.store(true);
    sessionPeakResetRequested.store(true);
}

void FF360MeterProcessor::resetHistogram()
{
    triggerHistogramReset.store(true);
}

bool FF360MeterProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* FF360MeterProcessor::createEditor()
{
    return new FF360MeterEditor (*this);
}

void FF360MeterProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void FF360MeterProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));

            const int fftOrder = apvts.state.getProperty ("fftOrder", static_cast<int> (FFTResolution::Mid));
            spectrumDSP.setFFTResolution (static_cast<FFTResolution> (fftOrder));
        }
}

#include "LoudnessTarget.h"

juce::AudioProcessorValueTreeState::ParameterLayout FF360MeterProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "targetProfile", 1 },
        "Target Profile",
        ff360_labs::LoudnessTarget::getPresetNames(),
        0
    ));

    juce::StringArray vuChoices;
    for (const auto& preset : VuDSP::getCalibrationPresets())
        vuChoices.add(preset.name);

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "vuRefLevel", 1 },
        "VU Calibration Reference",
        vuChoices,
        0 // Default to 0: -18 dBFS (Broadcast / SMPTE)
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "colorblindMode", 1 },
        "Colorblind Mode",
        false
    ));
    
    return layout;
}

// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FF360MeterProcessor();
}
