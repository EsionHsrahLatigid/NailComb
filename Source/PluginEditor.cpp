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
    setSize(defaultWidth, defaultHeight);
    setResizeLimits(minimumWidth, minimumHeight, defaultWidth * 2, defaultHeight * 2);
    setResizable(true, true);
    setName("NailComb editor");
    setComponentID("nailcomb-editor");
    setTitle("NailComb");
    setDescription("NailComb monochrome 8-bit custom editor");
    setWantsKeyboardFocus(true);

    for (std::size_t i = 0; i < controls.size(); ++i)
    {
        auto& slider = sliders[i];
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 112, 24);
        slider.setName(controls[i].name);
        slider.setComponentID(juce::String("nailcomb-") + controls[i].id);
        slider.setTooltip(controls[i].tip);
        slider.setWantsKeyboardFocus(true);
        addAndMakeVisible(slider);

        auto& label = labels[i];
        label.setText(controls[i].name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centredLeft);
        label.setColour(juce::Label::textColourId, juce::Colour(0xffeeeeee));
        label.setTooltip(controls[i].tip);
        label.attachToComponent(&slider, true);
        addAndMakeVisible(label);

        attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(ownerProcessor.parameters, controls[i].id, slider);
    }
}

void NailCombAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto area = getLocalBounds();
    g.fillAll(juce::Colour(0xff050505));

    const auto grid = 8;
    g.setColour(juce::Colour(0xff202020));
    for (int x = 0; x < area.getWidth(); x += grid)
        g.drawVerticalLine(x, 0.0f, static_cast<float>(area.getHeight()));
    for (int y = 0; y < area.getHeight(); y += grid)
        g.drawHorizontalLine(y, 0.0f, static_cast<float>(area.getWidth()));

    const auto frequency = ownerProcessor.parameters.getRawParameterValue(nailcomb::parameters::frequency)->load();
    const auto feedback = ownerProcessor.parameters.getRawParameterValue(nailcomb::parameters::feedback)->load();
    const auto damping = ownerProcessor.parameters.getRawParameterValue(nailcomb::parameters::damping)->load();
    const auto detune = ownerProcessor.parameters.getRawParameterValue(nailcomb::parameters::stereoDetune)->load();
    const auto spread = ownerProcessor.parameters.getRawParameterValue(nailcomb::parameters::voiceSpread)->load();
    const auto mix = ownerProcessor.parameters.getRawParameterValue(nailcomb::parameters::mix)->load();

    g.setColour(juce::Colour(0xffe8e8e8));
    g.setFont(juce::FontOptions(32.0f, juce::Font::bold));
    g.drawText("NailComb", 32, 24, area.getWidth() - 64, 48, juce::Justification::centredLeft);
    g.setFont(juce::FontOptions(16.0f));
    g.drawText("jp.ehl.nailcomb / NlCb", 34, 74, area.getWidth() - 68, 24, juce::Justification::centredLeft);

    const int motifLeft = 32;
    const int motifTop = 108;
    const int motifWidth = area.getWidth() - 64;
    const float normalizedFrequency = juce::jlimit(0.0f, 1.0f, (frequency - 20.0f) / (5000.0f - 20.0f));
    const float values[] {
        normalizedFrequency,
        feedback / nailcomb::dsp::NailCombDSP::maximumFeedback,
        damping,
        (detune + 50.0f) / 100.0f,
        spread,
        mix
    };
    for (int i = 0; i < 6; ++i)
    {
        const int y = motifTop + i * 20;
        const int filled = static_cast<int>(static_cast<float>(motifWidth) * values[i]);
        g.setColour(juce::Colour(i % 2 == 0 ? 0xffd6d6d6 : 0xff8a8a8a));
        for (int x = 0; x < filled; x += 16)
            g.fillRect(motifLeft + x, y, 8, 10);
        g.setColour(juce::Colour(0xff404040));
        g.drawRect(motifLeft, y, motifWidth, 10, 1);
    }

    g.setColour(juce::Colour(0xfff2f2f2));
    for (int x = 32; x < area.getWidth() - 32; x += 24)
    {
        const int h = 20 + ((x / 24) % 11) * 10;
        const int toothTop = area.getHeight() - 40 - h;
        g.fillRect(x, toothTop, 8, h);
        g.fillRect(x - 4, toothTop - 8, 16, 8);
    }
}

void NailCombAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(32);
    area.removeFromTop(192);
    const int rowHeight = 28;
    const int gap = 8;
    for (auto& slider : sliders)
    {
        slider.setBounds(area.removeFromTop(rowHeight).withTrimmedLeft(132));
        area.removeFromTop(gap);
    }
}
