#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "ParameterIDs.h"

namespace
{
struct ControlSpec
{
    const char* id;
    const char* name;
    const char* tip;
};

constexpr std::array<ControlSpec, 11> controls {{
    { nailcomb::parameters::frequency, "Frequency", "Tuned comb fundamental in hertz." },
    { nailcomb::parameters::fine, "Fine", "Cent offset around the tuned frequency." },
    { nailcomb::parameters::feedback, "Feedback", "Sub-unity resonant loop amount." },
    { nailcomb::parameters::polarity, "Polarity", "Positive or negative comb feedback polarity." },
    { nailcomb::parameters::damping, "Damping", "One-pole loop damping that darkens resonant repeats." },
    { nailcomb::parameters::smear, "Smear", "Reserved spectral smear control for voice motion weighting." },
    { nailcomb::parameters::stereoDetune, "Stereo Detune", "Opposite cent offsets for left and right comb banks." },
    { nailcomb::parameters::crossCouple, "Cross-Couple", "Bounded stereo feedback coupling between channels." },
    { nailcomb::parameters::voiceSpread, "Voice Spread", "Cent spread across the four comb teeth." },
    { nailcomb::parameters::mix, "Mix", "Dry to comb-resonator blend." },
    { nailcomb::parameters::trim, "Trim", "Output trim in decibels after the wet mix." },
}};
} // namespace

NailCombAudioProcessorEditor::NailCombAudioProcessorEditor(NailCombAudioProcessor& p)
    : AudioProcessorEditor(&p), ownerProcessor(p),
      tooltipText("NailComb: four fractional feedback comb voices with polarity, damping, stereo detune, cross-coupling, mix, and trim.")
{
    setResizeLimits(minimumWidth, minimumHeight, defaultWidth * 2, defaultHeight * 2);
    setResizable(true, true);
    setName("NailComb editor");
    setComponentID("nailcomb-editor");
    setTitle("NailComb");
    setDescription("NailComb monochrome 8-bit custom editor");
    setWantsKeyboardFocus(true);
    setLookAndFeel(&lookAndFeel);

    parameterDisplay.setComponentID("nailcomb-parameter-display");
    addAndMakeVisible(parameterDisplay);

    for (std::size_t i = 0; i < controls.size(); ++i)
    {
        auto& slider = sliders[i];
        slider.setName(controls[i].name);
        slider.setComponentID(juce::String("nailcomb-") + controls[i].id);
        slider.setTooltip(controls[i].tip);
        slider.setTitle(controls[i].name);
        slider.setDescription(controls[i].tip);
        slider.setLookAndFeel(&lookAndFeel);
        ehl::juce_design::styleSlider(slider);
        addAndMakeVisible(slider);

        auto& label = labels[i];
        label.setText(controls[i].name, juce::dontSendNotification);
        label.setName(juce::String(controls[i].name) + " label");
        label.setComponentID(juce::String("nailcomb-label-") + controls[i].id);
        label.setTitle(juce::String(controls[i].name) + " label");
        label.setDescription(controls[i].tip);
        label.setJustificationType(juce::Justification::centredLeft);
        label.setTooltip(controls[i].tip);
        label.setLookAndFeel(&lookAndFeel);
        ehl::juce_design::styleLabel(label);
        addAndMakeVisible(label);

        attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(ownerProcessor.parameters, controls[i].id, slider);
    }

    setSize(defaultWidth, defaultHeight);
    timerCallback();
    startTimerHz(30);
}

NailCombAudioProcessorEditor::~NailCombAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
    for (auto& label : labels)
        label.setLookAndFeel(nullptr);
    for (auto& slider : sliders)
        slider.setLookAndFeel(nullptr);
}

void NailCombAudioProcessorEditor::paint(juce::Graphics& g)
{
    ehl::juce_design::paintEditorChrome(g, getLocalBounds(), "NailComb", "COMB FILTER");
}

void NailCombAudioProcessorEditor::resized()
{
    parameterDisplay.setBounds(ehl::juce_design::parameterDisplayArea(getLocalBounds()));

    for (std::size_t i = 0; i < sliders.size(); ++i)
        ehl::juce_design::layoutLabelledControl(labels[i], sliders[i],
                                                ehl::juce_design::controlCell(getLocalBounds(), i));
}

void NailCombAudioProcessorEditor::timerCallback()
{
    parameterDisplay.setValues({
        static_cast<float>(sliders[0].valueToProportionOfLength(sliders[0].getValue())),
        static_cast<float>(sliders[2].valueToProportionOfLength(sliders[2].getValue())),
        static_cast<float>(sliders[7].valueToProportionOfLength(sliders[7].getValue())),
        static_cast<float>(sliders[8].valueToProportionOfLength(sliders[8].getValue())),
    });
}
