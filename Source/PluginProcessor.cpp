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

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "waveform", 1 },
        "Waveform",
        juce::StringArray { "Sine", "Saw", "Square" },
        0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "lfoRate", 1 },
        "LFO Rate",
        juce::NormalisableRange<float> (0.1f, 20.0f, 0.01f),
        2.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "lfoDepth", 1 },
        "LFO Depth",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "lfoTarget", 1 },
        "LFO Target",
        juce::StringArray { "Amplitude", "Pitch", "Cutoff" },
        0));

    return { params.begin(), params.end() };
}

//==============================================================================
void SineSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    currentSampleRate = sampleRate;
    phase = 0.0;
    lfoPhase = 0.0;

    smoothedGain.reset (sampleRate, 0.05);
    smoothedFrequency.reset (sampleRate, 0.02);
    smoothedCutoff.reset (sampleRate, 0.05);
    smoothedLfoRate.reset (sampleRate, 0.05);
    smoothedLfoDepth.reset (sampleRate, 0.05);

    smoothedGain.setCurrentAndTargetValue (apvts.getRawParameterValue ("gain")->load());
    smoothedFrequency.setCurrentAndTargetValue (apvts.getRawParameterValue ("frequency")->load());
    smoothedCutoff.setCurrentAndTargetValue (apvts.getRawParameterValue ("cutoff")->load());
    smoothedLfoRate.setCurrentAndTargetValue (apvts.getRawParameterValue ("lfoRate")->load());
    smoothedLfoDepth.setCurrentAndTargetValue (apvts.getRawParameterValue ("lfoDepth")->load());

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
    smoothedLfoRate.setTargetValue (apvts.getRawParameterValue ("lfoRate")->load());
    smoothedLfoDepth.setTargetValue (apvts.getRawParameterValue ("lfoDepth")->load());

    adsrParams.attack = apvts.getRawParameterValue ("attack")->load();
    adsrParams.decay = apvts.getRawParameterValue ("decay")->load();
    adsrParams.sustain = apvts.getRawParameterValue ("sustain")->load();
    adsrParams.release = apvts.getRawParameterValue ("release")->load();
    adsr.setParameters (adsrParams);

    const bool gateOn = apvts.getRawParameterValue ("gate")->load() > 0.5f;
    const auto waveform = static_cast<int> (apvts.getRawParameterValue ("waveform")->load());
    const auto lfoTarget = static_cast<int> (apvts.getRawParameterValue ("lfoTarget")->load());

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
        const auto currentLfoRate = smoothedLfoRate.getNextValue();
        const auto currentLfoDepth = smoothedLfoDepth.getNextValue();

        const float lfoRaw = std::sin (lfoPhase);

        auto modulatedFrequency = currentFrequency;

        if (lfoTarget == 1)
            modulatedFrequency *= 1.0f + (lfoRaw * currentLfoDepth * 0.1f);

        auto modulatedCutoff = currentCutoff;

        if (lfoTarget == 2)
            modulatedCutoff *= 1.0f + (lfoRaw * currentLfoDepth);

        const auto limitedCutoff = juce::jlimit (20.0f,
                                                 20000.0f,
                                                 juce::jlimit (20.0f,
                                                               static_cast<float> (currentSampleRate * 0.5 - 1.0),
                                                               modulatedCutoff));
        const auto coefficients = juce::IIRCoefficients::makeLowPass (currentSampleRate, limitedCutoff);

        for (auto& filter : filters)
            filter.setCoefficients (coefficients);

        const float env = adsr.getNextSample();

        float oscillatorSample = std::sin ((float) phase);

        if (waveform == 1)
            oscillatorSample = static_cast<float> ((phase / juce::MathConstants<double>::pi) - 1.0);
        else if (waveform == 2)
            oscillatorSample = phase < juce::MathConstants<double>::pi ? 1.0f : -1.0f;

        float output = currentGain * oscillatorSample * env;

        if (lfoTarget == 0)
        {
            const float lfoUnipolar = 0.5f + 0.5f * lfoRaw;
            const float lfoMultiplier = juce::jmap (currentLfoDepth, 0.0f, 1.0f, 1.0f, lfoUnipolar);
            output *= lfoMultiplier;
        }

        for (int channel = 0; channel < totalNumOutputChannels; ++channel)
        {
            auto filteredOutput = filters[static_cast<size_t> (channel)].processSingleSampleRaw (output);
            buffer.setSample (channel, sample, filteredOutput);
        }

        phase += 2.0 * juce::MathConstants<double>::pi * modulatedFrequency / currentSampleRate;
        lfoPhase += 2.0 * juce::MathConstants<double>::pi * currentLfoRate / currentSampleRate;

        if (phase >= 2.0 * juce::MathConstants<double>::pi)
            phase -= 2.0 * juce::MathConstants<double>::pi;

        if (lfoPhase >= 2.0 * juce::MathConstants<double>::pi)
            lfoPhase -= 2.0 * juce::MathConstants<double>::pi;
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
