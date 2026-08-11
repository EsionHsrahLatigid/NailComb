#include "TestSupport.h"
#include "ParameterIDs.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <juce_events/juce_events.h>

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
        }

        juce::Image image(juce::Image::RGB, 320, 220, true);
        juce::Graphics g(image);
        editor->setBounds(0, 0, image.getWidth(), image.getHeight());
        editor->paint(g);
        const auto first = image.getPixelAt(0, 0);
        bool varied = false;
        bool hasBrightTooth = false;
        for (int y = 0; y < image.getHeight(); y += 12)
            for (int x = 0; x < image.getWidth(); x += 12)
            {
                const auto pixel = image.getPixelAt(x, y);
                varied = varied || pixel != first;
                hasBrightTooth = hasBrightTooth || pixel.getBrightness() > 0.75f;
            }
        test_support::check(varied && hasBrightTooth, "software paint uses grayscale nail and teeth motif");
    });
}
