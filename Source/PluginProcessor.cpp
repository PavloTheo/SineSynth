/*

  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================

*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SineSynthAudioProcessor::SineSynthAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
        apvts (*this, nullptr, "Parameters", createParameterLayout())
#endif
{
}

SineSynthAudioProcessor::~SineSynthAudioProcessor()
{
}

//==============================================================================
const juce::String SineSynthAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SineSynthAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SineSynthAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SineSynthAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SineSynthAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SineSynthAudioProcessor::getNumPrograms()
{
    return 1;
}

int SineSynthAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SineSynthAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String SineSynthAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void SineSynthAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

juce::AudioProcessorValueTreeState::ParameterLayout SineSynthAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "gain", 1 },
        "Gain",
        juce::NormalisableRange<float> (0.0f, 0.25f, 0.001f),
        0.1f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "frequency", 1 },
        "Frequency",
        juce::NormalisableRange<float> (20.0f, 2000.0f, 1.0f),
        440.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "gate", 1 },
        "Gate",
        false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "attack", 1 },
        "Attack",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.5f),
        0.1f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "decay", 1 },
        "Decay",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.5f),
        0.2f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sustain", 1 },
        "Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.8f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "release", 1 },
        "Release",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.5f),
        0.4f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "cutoff", 1 },
        "Cutoff",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.5f),
        20000.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
void SineSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    currentSampleRate = sampleRate;
    phase = 0.0;

    smoothedGain.reset (sampleRate, 0.05);
    smoothedFrequency.reset (sampleRate, 0.02);
    smoothedCutoff.reset (sampleRate, 0.05);

    smoothedGain.setCurrentAndTargetValue (apvts.getRawParameterValue ("gain")->load());
    smoothedFrequency.setCurrentAndTargetValue (apvts.getRawParameterValue ("frequency")->load());
    smoothedCutoff.setCurrentAndTargetValue (apvts.getRawParameterValue ("cutoff")->load());

    adsr.setSampleRate (sampleRate);

    const auto initialCutoff = juce::jlimit (20.0f,
                                             static_cast<float> (sampleRate * 0.5 - 1.0),
                                             smoothedCutoff.getCurrentValue());
    const auto coefficients = juce::IIRCoefficients::makeLowPass (sampleRate, initialCutoff);

    for (auto& filter : filters)
    {
        filter.reset();
        filter.setCoefficients (coefficients);
    }
}

void SineSynthAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SineSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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
#endif

void SineSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();
    const auto numSamples = buffer.getNumSamples();

    smoothedGain.setTargetValue (apvts.getRawParameterValue ("gain")->load());
    smoothedFrequency.setTargetValue (apvts.getRawParameterValue ("frequency")->load());
    smoothedCutoff.setTargetValue (apvts.getRawParameterValue ("cutoff")->load());

    adsrParams.attack = apvts.getRawParameterValue ("attack")->load();
    adsrParams.decay = apvts.getRawParameterValue ("decay")->load();
    adsrParams.sustain = apvts.getRawParameterValue ("sustain")->load();
    adsrParams.release = apvts.getRawParameterValue ("release")->load();
    adsr.setParameters (adsrParams);

    const bool gateOn = apvts.getRawParameterValue ("gate")->load() > 0.5f;

    if (gateOn && ! wasGateOn)
        adsr.noteOn();
    else if (! gateOn && wasGateOn)
        adsr.noteOff();

    wasGateOn = gateOn;

    for (auto ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear (ch, 0, numSamples);

    if (currentSampleRate <= 0.0)
        return;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto currentGain = smoothedGain.getNextValue();
        const auto currentFrequency = smoothedFrequency.getNextValue();
        const auto currentCutoff = smoothedCutoff.getNextValue();

        const auto limitedCutoff = juce::jlimit (20.0f,
                                                 static_cast<float> (currentSampleRate * 0.5 - 1.0),
                                                 currentCutoff);
        const auto coefficients = juce::IIRCoefficients::makeLowPass (currentSampleRate, limitedCutoff);

        for (auto& filter : filters)
            filter.setCoefficients (coefficients);

        const float env = adsr.getNextSample();
        const float output = currentGain * std::sin ((float) phase) * env;

        for (int channel = 0; channel < totalNumOutputChannels; ++channel)
        {
            auto filteredOutput = filters[static_cast<size_t> (channel)].processSingleSampleRaw (output);
            buffer.setSample (channel, sample, filteredOutput);
        }

        phase += 2.0 * juce::MathConstants<double>::pi * currentFrequency / currentSampleRate;

        if (phase >= 2.0 * juce::MathConstants<double>::pi)
            phase -= 2.0 * juce::MathConstants<double>::pi;
    }
}

//==============================================================================
bool SineSynthAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* SineSynthAudioProcessor::createEditor()
{
    return new SineSynthAudioProcessorEditor (*this);
}

//==============================================================================
void SineSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused (destData);
}

void SineSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SineSynthAudioProcessor();
}
