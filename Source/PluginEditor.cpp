/*

  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================

*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SineSynthAudioProcessorEditor::SineSynthAudioProcessorEditor (SineSynthAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    gainSlider.setSliderStyle (juce::Slider::Rotary);
    gainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible (gainSlider);

    frequencySlider.setSliderStyle (juce::Slider::Rotary);
    frequencySlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible (frequencySlider);

    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "gain", gainSlider);

    frequencyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "frequency", frequencySlider);

    gateButton.setButtonText ("Gate");
    addAndMakeVisible (gateButton);

    attackSlider.setSliderStyle (juce::Slider::Rotary);
    attackSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible (attackSlider);

    decaySlider.setSliderStyle (juce::Slider::Rotary);
    decaySlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible (decaySlider);

    sustainSlider.setSliderStyle (juce::Slider::Rotary);
    sustainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible (sustainSlider);

    releaseSlider.setSliderStyle (juce::Slider::Rotary);
    releaseSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible (releaseSlider);

    gateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.getAPVTS(), "gate", gateButton);

    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "attack", attackSlider);

    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "decay", decaySlider);

    sustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "sustain", sustainSlider);

    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "release", releaseSlider);

    setSize (420, 300);
}

SineSynthAudioProcessorEditor::~SineSynthAudioProcessorEditor()
{
}

//==============================================================================
void SineSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (juce::Colours::black);

    g.setColour (juce::Colours::white);
    g.setFont (24.0f);
    g.drawFittedText ("SineSynth", 0, 20, getWidth(), 30, juce::Justification::centred, 1);
}

void SineSynthAudioProcessorEditor::resized ()
{
    gateButton.setBounds (20, 20, 80, 30);

    gainSlider.setBounds (20, 70, 100, 100);
    frequencySlider.setBounds (140, 70, 100, 100);

    attackSlider.setBounds (20, 190, 80, 80);
    decaySlider.setBounds (110, 190, 80, 80);
    sustainSlider.setBounds (200, 190, 80, 80);
    releaseSlider.setBounds (290, 190, 80, 80);
}
