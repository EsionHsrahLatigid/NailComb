#include "TestSupport.h"
#include "ParameterIDs.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <ehl/juce_design/EhlDesign.h>

#include <juce_events/juce_events.h>

#include <algorithm>
#include <array>
#include <iterator>

struct EditorTestAccess
{
    static void refresh(NailCombAudioProcessorEditor& editor) { editor.timerCallback(); }
};

namespace
{
void checkSharedChromePaint(juce::AudioProcessorEditor& editor)
{
    const auto background = ehl::juce_design::Palette::ink();
    const auto divider = ehl::juce_design::Palette::low();
    const auto paper = ehl::juce_design::Palette::paper();
    juce::Image image(juce::Image::RGB, editor.getWidth(), editor.getHeight(), true);
    {
        juce::Graphics g(image);
        editor.paint(g);
    }

    bool headerHasInk = false;
    bool dividerIsExact = true;
    bool bodyIsBackground = true;
    bool topStripIsPaper = true;
    bool neutral = true;

    for (int y = 0; y < image.getHeight(); ++y)
    {
        for (int x = 0; x < image.getWidth(); ++x)
        {
            const auto pixel = image.getPixelAt(x, y);
            // The EHL paper/mid tones are warm neutral (#F2F2F0/#8A8A86), so
            // monochrome allows a four-count channel spread while rejecting accents.
            const auto minChannel = std::min({ pixel.getRed(), pixel.getGreen(), pixel.getBlue() });
            const auto maxChannel = std::max({ pixel.getRed(), pixel.getGreen(), pixel.getBlue() });
            neutral = neutral && static_cast<int>(maxChannel) - static_cast<int>(minChannel) <= 4;

            if (y < 4)
                topStripIsPaper = topStripIsPaper && pixel == paper;
            else if (y < ehl::juce_design::Metrics::dividerY)
                headerHasInk = headerHasInk || pixel != background;
            else if (y == ehl::juce_design::Metrics::dividerY)
            {
                const bool onDivider = x >= ehl::juce_design::Metrics::margin
                                    && x < image.getWidth() - ehl::juce_design::Metrics::margin;
                dividerIsExact = dividerIsExact && pixel == (onDivider ? divider : background);
            }
            else if (y >= ehl::juce_design::Metrics::headerHeight)
                bodyIsBackground = bodyIsBackground && pixel == background;
        }
    }

    test_support::check(topStripIsPaper, "paint draws shared paper top strip at y=0..3");
    test_support::check(image.getPixelAt(0, 4) == background, "paint background is ink below top strip");
    test_support::check(headerHasInk, "paint keeps product identity above the divider");
    test_support::check(dividerIsExact, "paint draws the shared divider at y=60 from x=16 through width-17");
    test_support::check(bodyIsBackground, "editor paint draws no direct body content below shared chrome");
    test_support::check(neutral, "paint stays in the neutral monochrome ramp");
}

void checkControlContract(juce::AudioProcessorEditor& editor, const char* parameterID, std::size_t index)
{
    const juce::String id(parameterID);
    auto* label = dynamic_cast<juce::Label*>(editor.findChildWithID("nailcomb-label-" + id));
    auto* control = dynamic_cast<juce::Slider*>(editor.findChildWithID("nailcomb-" + id));
    test_support::check(label != nullptr, "label exists: " + id.toStdString());
    test_support::check(control != nullptr, "control exists: " + id.toStdString());

    const auto expected = ehl::juce_design::labelledControlBounds(ehl::juce_design::controlCell(editor.getLocalBounds(), index));
    test_support::check(label->getBounds() == expected.label, "label uses shared grid: " + id.toStdString());
    test_support::check(control->getBounds() == expected.control, "control uses shared grid: " + id.toStdString());
    test_support::check(control->getY() >= ehl::juce_design::Metrics::headerHeight, "control starts at or below y=64: " + id.toStdString());
    test_support::check(control->getRight() <= editor.getWidth(), "control fits editor width: " + id.toStdString());
    test_support::check(control->getBottom() <= editor.getHeight(), "control fits editor height: " + id.toStdString());
    test_support::check(control->getSliderStyle() == juce::Slider::RotaryHorizontalVerticalDrag, "control uses shared rotary style: " + id.toStdString());
    test_support::check(control->getTextBoxPosition() == juce::Slider::TextBoxBelow, "control uses shared value placement: " + id.toStdString());
    test_support::check(control->getTextBoxWidth() == ehl::juce_design::Metrics::valueWidth, "control uses shared value width: " + id.toStdString());
    test_support::check(control->findColour(juce::Slider::thumbColourId) == ehl::juce_design::Palette::paper(), "control uses shared paper thumb: " + id.toStdString());
    test_support::check(control->findColour(juce::Slider::trackColourId) == ehl::juce_design::Palette::mid(), "control uses shared mid track: " + id.toStdString());
    test_support::check(control->findColour(juce::Slider::backgroundColourId) == ehl::juce_design::Palette::low(), "control uses shared low background: " + id.toStdString());
}

void checkAllControlContracts(juce::AudioProcessorEditor& editor)
{
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
    for (std::size_t i = 0; i < std::size(ids); ++i)
        checkControlContract(editor, ids[i], i);
}

ehl::juce_design::ParameterDisplay& checkParameterDisplay(juce::AudioProcessorEditor& editor)
{
    auto* display = dynamic_cast<ehl::juce_design::ParameterDisplay*>(editor.findChildWithID("nailcomb-parameter-display"));
    test_support::check(display != nullptr, "parameter display exists");
    test_support::check(display->getKind() == ehl::juce_design::DisplayKind::comb, "parameter display kind is comb");
    test_support::check(display->getBounds() == ehl::juce_design::parameterDisplayArea(editor.getLocalBounds()), "parameter display uses shared bounds");
    test_support::check(! display->getWantsKeyboardFocus(), "parameter display is noninteractive");

    bool interceptsSelf = true;
    bool interceptsChildren = true;
    display->getInterceptsMouseClicks(interceptsSelf, interceptsChildren);
    test_support::check(! interceptsSelf && ! interceptsChildren, "parameter display ignores mouse clicks");
    return *display;
}

float normalizedSliderValue(juce::Slider& slider)
{
    return static_cast<float>(slider.valueToProportionOfLength(slider.getValue()));
}

bool valuesDiffer(float a, float b)
{
    return std::abs(a - b) > 0.0001f;
}

void dispatchEditorTimer(juce::AudioProcessorEditor& editor)
{
    auto* custom = dynamic_cast<NailCombAudioProcessorEditor*>(&editor);
    test_support::check(custom != nullptr, "custom editor is available for display refresh");
    EditorTestAccess::refresh(*custom);
}

void checkDisplayValuesTrackSliders(juce::AudioProcessorEditor& editor)
{
    auto& display = checkParameterDisplay(editor);
    auto* frequency = dynamic_cast<juce::Slider*>(editor.findChildWithID("nailcomb-frequency"));
    auto* feedback = dynamic_cast<juce::Slider*>(editor.findChildWithID("nailcomb-feedback"));
    auto* crossCouple = dynamic_cast<juce::Slider*>(editor.findChildWithID("nailcomb-crossCouple"));
    auto* voiceSpread = dynamic_cast<juce::Slider*>(editor.findChildWithID("nailcomb-voiceSpread"));
    test_support::check(frequency != nullptr && feedback != nullptr && crossCouple != nullptr && voiceSpread != nullptr, "display source sliders exist");

    dispatchEditorTimer(editor);
    const std::array<float, 4> expected {
        normalizedSliderValue(*frequency),
        normalizedSliderValue(*feedback),
        normalizedSliderValue(*crossCouple),
        normalizedSliderValue(*voiceSpread),
    };
    test_support::check(display.getValues() == expected, "parameter display values mirror attached sliders");

    const auto before = display.getValues();
    crossCouple->setValue(crossCouple->getMaximum(), juce::dontSendNotification);
    dispatchEditorTimer(editor);
    test_support::check(valuesDiffer(display.getValues()[2], before[2]), "changing cross-couple slider updates display value");
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juce;
    return test_support::run("nailcomb_editor_tests", [] {
        NailCombAudioProcessor processor;
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        auto* custom = dynamic_cast<NailCombAudioProcessorEditor*>(editor.get());
        test_support::check(custom != nullptr, "custom editor type, not GenericAudioProcessorEditor");
        test_support::check(dynamic_cast<juce::GenericAudioProcessorEditor*>(editor.get()) == nullptr, "not GenericAudioProcessorEditor");
        test_support::check(editor->getWidth() == ehl::juce_design::Metrics::defaultWidth, "default width");
        test_support::check(editor->getHeight() == ehl::juce_design::Metrics::defaultHeight, "default height");
        test_support::check(NailCombAudioProcessorEditor::minimumWidth == ehl::juce_design::Metrics::minimumWidth, "minimum width");
        test_support::check(NailCombAudioProcessorEditor::minimumHeight == ehl::juce_design::Metrics::minimumHeight, "minimum height");
        test_support::check(editor->getComponentID() == "nailcomb-editor", "component id");
        test_support::check(editor->getName().isNotEmpty(), "accessible name");
        test_support::check(custom->getTooltip().isNotEmpty(), "editor tooltip");
        test_support::check(editor->getWantsKeyboardFocus(), "keyboard focus");

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
        {
            auto* control = editor->findChildWithID(juce::String("nailcomb-") + id);
            test_support::check(control != nullptr, std::string("editor control exists: ") + id);
            test_support::check(control->getName().isNotEmpty(), std::string("control has accessible name: ") + id);
            auto* slider = dynamic_cast<juce::Slider*>(control);
            test_support::check(slider != nullptr && slider->getTooltip().isNotEmpty(), std::string("control has tooltip: ") + id);
        }

        editor->setBounds(0, 0, ehl::juce_design::Metrics::defaultWidth, ehl::juce_design::Metrics::defaultHeight);
        checkParameterDisplay(*editor);
        checkAllControlContracts(*editor);
        checkDisplayValuesTrackSliders(*editor);
        checkSharedChromePaint(*editor);

        editor->setBounds(0, 0, ehl::juce_design::Metrics::minimumWidth, ehl::juce_design::Metrics::minimumHeight);
        checkParameterDisplay(*editor);
        checkAllControlContracts(*editor);
        checkSharedChromePaint(*editor);

        editor->setBounds(0, 0, ehl::juce_design::Metrics::maximumWidth, ehl::juce_design::Metrics::maximumHeight);
        checkParameterDisplay(*editor);
        checkAllControlContracts(*editor);
        checkSharedChromePaint(*editor);
    });
}
