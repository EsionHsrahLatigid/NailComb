#include "TestSupport.h"
#include "ParameterIDs.h"
#include "PluginProcessor.h"

#include <juce_events/juce_events.h>
#include <cmath>

namespace
{
juce::AudioProcessor::BusesLayout layout(juce::AudioChannelSet input, juce::AudioChannelSet output)
{
    juce::AudioProcessor::BusesLayout buses;
    buses.inputBuses.add(input);
    buses.outputBuses.add(output);
    return buses;
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juce;
    return test_support::run("nailcomb_plugin_tests", [] {
        NailCombAudioProcessor processor;
        test_support::check(processor.getName() == "NailComb", "product name");
        test_support::check(!processor.acceptsMidi(), "audio effect does not require MIDI");
        test_support::check(!processor.isMidiEffect(), "not a MIDI effect");
        test_support::check(processor.getLatencySamples() == 0, "zero latency comb effect");
        test_support::check(processor.getTailLengthSeconds() > 0.5, "feedback comb declares a tail");

        test_support::check(processor.isBusesLayoutSupported(layout(juce::AudioChannelSet::mono(), juce::AudioChannelSet::mono())), "mono to mono bus supported");
        test_support::check(processor.isBusesLayoutSupported(layout(juce::AudioChannelSet::stereo(), juce::AudioChannelSet::stereo())), "stereo to stereo bus supported");
        test_support::check(!processor.isBusesLayoutSupported(layout(juce::AudioChannelSet::mono(), juce::AudioChannelSet::stereo())), "mono to stereo bus rejected");
        test_support::check(!processor.isBusesLayoutSupported(layout(juce::AudioChannelSet::stereo(), juce::AudioChannelSet::mono())), "stereo to mono bus rejected");

        const char* ids[] {
            nailcomb::parameters::frequency,
            nailcomb::parameters::fine,
            nailcomb::parameters::feedback,
            nailcomb::parameters::polarity,
            nailcomb::parameters::damping,
            nailcomb::parameters::smear,
            nailcomb::parameters::stereoDetune,
            nailcomb::parameters::crossCouple,
            nailcomb::parameters::voiceSpread,
            nailcomb::parameters::mix,
            nailcomb::parameters::trim
        };
        for (const auto* id : ids)
            test_support::check(processor.parameters.getParameter(id) != nullptr, std::string("parameter exists: ") + id);

        auto* frequency = processor.parameters.getParameter(nailcomb::parameters::frequency);
        frequency->setValueNotifyingHost(frequency->convertTo0to1(777.0f));
        auto* feedback = processor.parameters.getParameter(nailcomb::parameters::feedback);
        feedback->setValueNotifyingHost(feedback->convertTo0to1(0.88f));

        juce::MemoryBlock state;
        processor.getStateInformation(state);
        frequency->setValueNotifyingHost(frequency->convertTo0to1(120.0f));
        feedback->setValueNotifyingHost(feedback->convertTo0to1(0.2f));
        processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        test_support::check(std::abs(processor.parameters.getRawParameterValue(nailcomb::parameters::frequency)->load() - 777.0f) < 2.0f, "frequency state round-trip");
        test_support::check(std::abs(processor.parameters.getRawParameterValue(nailcomb::parameters::feedback)->load() - 0.88f) < 0.02f, "feedback state round-trip");

        const char invalid[] = "not xml";
        processor.setStateInformation(invalid, static_cast<int>(sizeof(invalid)));
        test_support::check(std::isfinite(processor.parameters.getRawParameterValue(nailcomb::parameters::frequency)->load()), "invalid state ignored safely");

        processor.prepareToPlay(48000.0, 64);
        processor.parameters.getParameter(nailcomb::parameters::frequency)->setValueNotifyingHost(frequency->convertTo0to1(1000.0f));
        processor.parameters.getParameter(nailcomb::parameters::feedback)->setValueNotifyingHost(feedback->convertTo0to1(0.8f));
        juce::AudioBuffer<float> buffer(2, 160);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            buffer.setSample(0, i, i == 0 ? 1.0f : 0.0f);
            buffer.setSample(1, i, 0.0f);
        }
        juce::MidiBuffer midi;
        processor.processBlock(buffer, midi);
        float rightEnergy = 0.0f;
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                test_support::check(std::isfinite(buffer.getSample(ch, i)), "processed samples finite");
            rightEnergy += std::abs(buffer.getSample(1, i));
        }
        test_support::check(rightEnergy < 0.0001f, "silent right channel remains silent without cross input");
    });
}
