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

    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "gain", gainSlider);

    frequencyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "frequency", frequencySlider);

    setSize (400, 300);
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
    gainSlider.setBounds (60, 100, 120, 120);
    frequencySlider.setBounds (220, 100, 120, 120);
}
