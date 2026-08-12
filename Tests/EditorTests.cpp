#include "TestSupport.h"
#include "ParameterIDs.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <juce_events/juce_events.h>

#include <algorithm>

namespace
{
void checkSimpleHeaderOnlyPaint(juce::AudioProcessorEditor& editor)
{
    const auto background = juce::Colour(0xff050505);
    const auto divider = juce::Colour(0xff2a2a2a);
    constexpr int headerLimit = 80;

    juce::Image image(juce::Image::RGB, 320, 200, true);
    juce::Graphics g(image);
    editor.setBounds(0, 0, image.getWidth(), image.getHeight());
    editor.paint(g);

    bool headerHasInk = false;
    bool separatorBandIsSimple = true;
    bool bodyIsBackground = true;
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

            if (y < 64)
                headerHasInk = headerHasInk || pixel != background;
            else if (y < headerLimit)
            {
                const bool onDivider = y == 72 && x >= 32 && x < image.getWidth() - 32;
                separatorBandIsSimple = separatorBandIsSimple && pixel == (onDivider ? divider : background);
            }
            else if (y >= headerLimit)
                bodyIsBackground = bodyIsBackground && pixel == background;
        }
    }

    test_support::check(image.getPixelAt(0, 0) == background, "paint background is #050505");
    test_support::check(headerHasInk, "paint keeps product identity inside the 80px header");
    test_support::check(separatorBandIsSimple, "paint keeps only one divider in the 64px to 79px separator band");
    test_support::check(bodyIsBackground, "paint draws no decorative motif, grid, panel, meter, or visualizer below the 80px header");
    test_support::check(neutral, "paint stays in the neutral monochrome ramp");
}

void checkControlBounds(juce::AudioProcessorEditor& editor, const juce::String& componentID)
{
    auto* control = editor.findChildWithID(componentID);
    test_support::check(control != nullptr, "control exists: " + componentID.toStdString());

    const auto bounds = control->getBounds();
    test_support::check(!bounds.isEmpty(), "control has initial bounds: " + componentID.toStdString());
    test_support::check(bounds.getY() >= 80, "control starts below the 80px header: " + componentID.toStdString());
    test_support::check(bounds.getRight() <= editor.getWidth(), "control fits editor width: " + componentID.toStdString());
    test_support::check(bounds.getBottom() <= editor.getHeight(), "control fits editor height: " + componentID.toStdString());
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
        test_support::check(editor->getWidth() == NailCombAudioProcessorEditor::defaultWidth, "default width");
        test_support::check(editor->getHeight() == NailCombAudioProcessorEditor::defaultHeight, "default height");
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
            checkControlBounds(*editor, juce::String("nailcomb-") + id);
        }

        checkSimpleHeaderOnlyPaint(*editor);
    });
}
