#include "TestSupport.h"
#include "dsp/NailCombDSP.h"

#include <cmath>
#include <limits>
#include <vector>

namespace
{
nailcomb::dsp::NailCombParameters tuned(float frequency, float feedback = 0.6f)
{
    nailcomb::dsp::NailCombParameters p;
    p.frequencyHz = frequency;
    p.feedback = feedback;
    p.damping = 0.0f;
    p.smear = 0.0f;
    p.stereoDetuneCents = 0.0f;
    p.crossCouple = 0.0f;
    p.voiceSpread = 0.0f;
    p.mix = 1.0f;
    p.trimDb = 0.0f;
    return p;
}

std::vector<float> renderImpulse(nailcomb::dsp::NailCombDSP& dsp, int samples, int channel = 0)
{
    std::vector<float> out(static_cast<std::size_t>(samples));
    for (int i = 0; i < samples; ++i)
        out[static_cast<std::size_t>(i)] = dsp.processSample(i == 0 ? 1.0f : 0.0f, channel);
    return out;
}

float absEnergy(const std::vector<float>& samples, int first, int last)
{
    float total = 0.0f;
    for (int i = first; i < last; ++i)
        total += std::abs(samples[static_cast<std::size_t>(i)]);
    return total;
}
} // namespace

int main()
{
    return test_support::run("nailcomb_dsp_tests", [] {
        nailcomb::dsp::NailCombDSP dsp;
        dsp.prepare(48000.0, 512, 2);

        auto p = tuned(1000.0f, 0.55f);
        dsp.setTargets(p);
        dsp.reset();
        const auto impulse = renderImpulse(dsp, 160);
        test_support::check(std::abs(impulse[48]) > 0.75f, "1000 Hz comb repeat appears at 48 samples");
        test_support::check(std::abs(impulse[47]) < 0.05f && std::abs(impulse[49]) < 0.08f, "comb repeat is tightly timed");

        p.polarity = 1.0f;
        dsp.setTargets(p);
        dsp.reset();
        const auto positive = renderImpulse(dsp, 130);
        p.polarity = -1.0f;
        dsp.setTargets(p);
        dsp.reset();
        const auto negative = renderImpulse(dsp, 130);
        test_support::check(positive[96] > 0.2f, "positive feedback reinforces second repeat");
        test_support::check(negative[96] < -0.2f, "negative feedback inverts second repeat");

        p = tuned(600.0f, 0.8f);
        p.damping = 0.0f;
        dsp.setTargets(p);
        dsp.reset();
        const auto bright = renderImpulse(dsp, 600);
        p.damping = 1.0f;
        dsp.setTargets(p);
        dsp.reset();
        const auto damped = renderImpulse(dsp, 600);
        test_support::check(absEnergy(damped, 160, 560) < absEnergy(bright, 160, 560) * 0.55f, "damping lowers feedback-tail energy");

        p = tuned(440.0f, 0.5f);
        p.stereoDetuneCents = 24.0f;
        dsp.setTargets(p);
        dsp.reset();
        test_support::check(std::abs(dsp.currentDelaySamples(0, 0) - dsp.currentDelaySamples(1, 0)) > 0.5f, "stereo detune separates channel delays");

        p.feedback = 3.0f;
        p.crossCouple = 1.0f;
        dsp.setTargets(p);
        test_support::check(dsp.currentFeedbackMagnitude() <= nailcomb::dsp::NailCombDSP::maximumFeedback, "feedback target is capped below unity");

        dsp.prepare(44100.0, 257, 1);
        p = tuned(220.0f, 0.9f);
        p.mix = 1.0f;
        dsp.setTargets(p);
        dsp.reset();
        for (int i = 0; i < 4096; ++i)
            test_support::check(std::isfinite(dsp.processSample(i == 0 ? 1.0f : 0.0f, 0)), "long mono render remains finite");

        dsp.reset();
        for (int i = 0; i < 256; ++i)
            test_support::near(dsp.processSample(0.0f, 0), 0.0f, 0.0001f, "reset clears resonator memory");

        test_support::check(std::isfinite(dsp.processSample(std::numeric_limits<float>::infinity(), 0)), "infinite input sanitized");
        test_support::check(std::isfinite(dsp.processSample(std::numeric_limits<float>::quiet_NaN(), 1)), "NaN input sanitized");
    });
}
