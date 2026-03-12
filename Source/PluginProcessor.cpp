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
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SineSynthAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SineSynthAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SineSynthAudioProcessor::getProgramName (int index)
{
    return {};
}

void SineSynthAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
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

    smoothedGain.setCurrentAndTargetValue (apvts.getRawParameterValue ("gain")->load());
    smoothedFrequency.setCurrentAndTargetValue (apvts.getRawParameterValue ("frequency")->load());

    adsr.setSampleRate (sampleRate);
}

void SineSynthAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SineSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
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
        const float env = adsr.getNextSample();
        const float output = currentGain * std::sin ((float) phase) * env;

        for (int channel = 0; channel < totalNumOutputChannels; ++channel)
            buffer.setSample (channel, sample, output);

        phase += 2.0 * juce::MathConstants<double>::pi * currentFrequency / currentSampleRate;

        if (phase >= 2.0 * juce::MathConstants<double>::pi)
            phase -= 2.0 * juce::MathConstants<double>::pi;
    }
}

//==============================================================================
bool SineSynthAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SineSynthAudioProcessor::createEditor()
{
    return new SineSynthAudioProcessorEditor (*this);
}

//==============================================================================
void SineSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SineSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SineSynthAudioProcessor();
}
