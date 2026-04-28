/*

  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================

*/

#pragma once

#include <array>
#include <JuceHeader.h>

//==============================================================================
/**
*/
class SineSynthAudioProcessor  : public juce::AudioProcessor
{
public:
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==============================================================================
    SineSynthAudioProcessor();
    ~SineSynthAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    juce::AudioProcessorValueTreeState apvts;

    double currentSampleRate = 44100.0;
    double phase = 0.0;
    double lfoPhase = 0.0;

    juce::SmoothedValue<float> smoothedGain;
    juce::SmoothedValue<float> smoothedFrequency;
    juce::SmoothedValue<float> smoothedCutoff;
    juce::SmoothedValue<float> smoothedLfoRate;
    juce::SmoothedValue<float> smoothedLfoDepth;

    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams;
    bool wasGateOn = false;

    std::array<juce::IIRFilter, 2> filters;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SineSynthAudioProcessor)
};
