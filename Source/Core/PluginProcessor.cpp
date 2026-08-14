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
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Update VU Reference Level from APVTS
    if (auto* param = apvts.getRawParameterValue("vuRefLevel"))
    {
        int idx = (int)param->load();
        const auto& presets = VuDSP::getCalibrationPresets();
        if (idx >= 0 && idx < (int)presets.size())
            vuDSP.setReferenceLevelDb(presets[(size_t)idx].refDb);
    }

    // Input Signal & Device Activity Monitoring
    bool hasInputs = (totalNumInputChannels > 0 && buffer.getNumSamples() > 0);
    isInputConnected.store(hasInputs);

    float maxMag = hasInputs ? buffer.getMagnitude(0, buffer.getNumSamples()) : 0.0f;
    float peakDb = (maxMag > 1e-5f) ? (20.0f * std::log10(maxMag)) : -100.0f;
    currentPeakLevelDb.store(peakDb);
    isAudioSilent.store(maxMag < 0.0001f); // Lower than ~-80 dBFS considered idle silence

    // Calculate Peak and RMS for this block
    MeterData blockData = peakRmsDSP.processBlock(buffer);
    
    // Calculate VU for this block
    VuMeterData vuData = vuDSP.processBlock(buffer);
    
    // Calculate LUFS for this block
    LufsMeterData lufsData = lufsDSP.processBlock(buffer);
    
    // Calculate Phase Scope for this block
    PhaseScopeData phaseData = phaseScopeDSP.processBlock(buffer);
    
    // Calculate Spectrum for this block
    SpectrumData specData;
    bool newSpec = spectrumDSP.processBlock(buffer, specData);
    
    if (triggerHistogramReset.exchange(false))
    {
        histogramDSP.reset();
    }
    
    // Calculate Histogram for this block using Short-term LUFS
    HistogramData histData;
    bool newHist = histogramDSP.processBlock(buffer, lufsData.shortTerm, histData);
    
    // Push the struct safely to the GUI thread
    meterFifo.push(blockData);
    vuFifo.push(vuData);
    lufsFifo.push(lufsData);
    phaseScopeFifo.push(phaseData);
    if (newSpec) spectrumFifo.push(specData);
    if (newHist) histogramFifo.push(histData);
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
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
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
