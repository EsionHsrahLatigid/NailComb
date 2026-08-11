#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.h"

#include <cmath>
#include <vector>

NailCombAudioProcessor::NailCombAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout NailCombAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>(nailcomb::parameters::frequency, "Frequency", 20.0f, 5000.0f, 110.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(nailcomb::parameters::fine, "Fine", -100.0f, 100.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(nailcomb::parameters::feedback, "Feedback", 0.0f, nailcomb::dsp::NailCombDSP::maximumFeedback, 0.65f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(nailcomb::parameters::polarity, "Polarity", juce::StringArray { "Positive", "Negative" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(nailcomb::parameters::damping, "Damping", 0.0f, 1.0f, 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(nailcomb::parameters::smear, "Smear", 0.0f, 1.0f, 0.25f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(nailcomb::parameters::stereoDetune, "Stereo Detune", -50.0f, 50.0f, 7.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(nailcomb::parameters::crossCouple, "Cross-Couple", 0.0f, 0.35f, 0.12f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(nailcomb::parameters::voiceSpread, "Voice Spread", 0.0f, 1.0f, 0.4f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(nailcomb::parameters::mix, "Mix", 0.0f, 1.0f, 0.6f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(nailcomb::parameters::trim, "Trim", -24.0f, 6.0f, -3.0f));
    return { params.begin(), params.end() };
}

void NailCombAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    dsp.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

bool NailCombAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mainIn = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();
    return (mainIn == juce::AudioChannelSet::mono() && mainOut == juce::AudioChannelSet::mono())
        || (mainIn == juce::AudioChannelSet::stereo() && mainOut == juce::AudioChannelSet::stereo());
}

void NailCombAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    nailcomb::dsp::NailCombParameters targets;
    targets.frequencyHz = parameters.getRawParameterValue(nailcomb::parameters::frequency)->load();
    targets.fineCents = parameters.getRawParameterValue(nailcomb::parameters::fine)->load();
    targets.feedback = parameters.getRawParameterValue(nailcomb::parameters::feedback)->load();
    targets.polarity = parameters.getRawParameterValue(nailcomb::parameters::polarity)->load() < 0.5f ? 1.0f : -1.0f;
    targets.damping = parameters.getRawParameterValue(nailcomb::parameters::damping)->load();
    targets.smear = parameters.getRawParameterValue(nailcomb::parameters::smear)->load();
    targets.stereoDetuneCents = parameters.getRawParameterValue(nailcomb::parameters::stereoDetune)->load();
    targets.crossCouple = parameters.getRawParameterValue(nailcomb::parameters::crossCouple)->load();
    targets.voiceSpread = parameters.getRawParameterValue(nailcomb::parameters::voiceSpread)->load();
    targets.mix = parameters.getRawParameterValue(nailcomb::parameters::mix)->load();
    targets.trimDb = parameters.getRawParameterValue(nailcomb::parameters::trim)->load();
    dsp.setTargets(targets);

    const int totalIn = getTotalNumInputChannels();
    const int totalOut = getTotalNumOutputChannels();
    for (int channel = totalIn; channel < totalOut; ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

    for (int channel = 0; channel < totalOut; ++channel)
    {
        auto* samples = buffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            samples[sample] = dsp.processSample(samples[sample], channel);
    }
}

juce::AudioProcessorEditor* NailCombAudioProcessor::createEditor()
{
    return new NailCombAudioProcessorEditor(*this);
}

void NailCombAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = parameters.copyState().createXml())
        copyXmlToBinary(*state, destData);
}

void NailCombAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NailCombAudioProcessor();
}
