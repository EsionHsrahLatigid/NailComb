#include "dsp/NailCombDSP.h"

#include <algorithm>
#include <cmath>

namespace nailcomb::dsp
{
void NailCombDSP::prepare(double sampleRate, int maxBlockSize, int channels)
{
    sampleRate_ = std::isfinite(sampleRate) && sampleRate >= 8000.0 ? sampleRate : 44100.0;
    channels_ = clamp(static_cast<float>(channels), 0.0f, 2.0f) > 1.0f ? 2 : (channels > 0 ? 1 : 0);

    const auto longestDelay = static_cast<std::size_t>(std::ceil(sampleRate_ / minimumFrequencyHz));
    const auto guard = static_cast<std::size_t>(std::max(64, maxBlockSize < 0 ? 0 : maxBlockSize));
    resizeDelayLines(longestDelay + guard + 8);
    updateDerivedTargets();
    reset();
}

void NailCombDSP::reset() noexcept
{
    for (auto& channel : channelStates_)
    {
        channel.lastWet = 0.0f;
        for (auto& voice : channel.voices)
        {
            std::fill(voice.delay.begin(), voice.delay.end(), 0.0f);
            voice.writeIndex = 0;
            voice.damped = 0.0f;
        }
    }

    current_ = target_;
    currentDelays_ = targetDelays_;
}

void NailCombDSP::setTargets(const NailCombParameters& parameters) noexcept
{
    target_.frequencyHz = clamp(sanitize(parameters.frequencyHz), minimumFrequencyHz, maximumFrequencyHz);
    target_.fineCents = clamp(sanitize(parameters.fineCents), -100.0f, 100.0f);
    target_.feedback = clamp(sanitize(parameters.feedback), 0.0f, maximumFeedback);
    target_.polarity = sanitize(parameters.polarity) < 0.0f ? -1.0f : 1.0f;
    target_.damping = clamp(sanitize(parameters.damping), 0.0f, 1.0f);
    target_.smear = clamp(sanitize(parameters.smear), 0.0f, 1.0f);
    target_.stereoDetuneCents = clamp(sanitize(parameters.stereoDetuneCents), -50.0f, 50.0f);
    target_.crossCouple = clamp(sanitize(parameters.crossCouple), 0.0f, 0.35f);
    target_.voiceSpread = clamp(sanitize(parameters.voiceSpread), 0.0f, 1.0f);
    target_.mix = clamp(sanitize(parameters.mix), 0.0f, 1.0f);
    target_.trimDb = clamp(sanitize(parameters.trimDb), -24.0f, 6.0f);
    updateDerivedTargets();
}

float NailCombDSP::processSample(float input, int channel) noexcept
{
    input = clamp(sanitize(input), -8.0f, 8.0f);
    const int ch = channel <= 0 ? 0 : 1;
    const int other = ch == 0 ? 1 : 0;
    auto& state = channelStates_[ch];

    smooth(current_.frequencyHz, target_.frequencyHz);
    smooth(current_.fineCents, target_.fineCents);
    smooth(current_.feedback, target_.feedback);
    smooth(current_.polarity, target_.polarity);
    smooth(current_.damping, target_.damping);
    smooth(current_.smear, target_.smear);
    smooth(current_.stereoDetuneCents, target_.stereoDetuneCents);
    smooth(current_.crossCouple, target_.crossCouple);
    smooth(current_.voiceSpread, target_.voiceSpread);
    smooth(current_.mix, target_.mix);
    smooth(current_.trimDb, target_.trimDb);

    const float feedback = clamp(current_.feedback, 0.0f, maximumFeedback);
    const float polarity = current_.polarity < 0.0f ? -1.0f : 1.0f;
    const float cross = clamp(current_.crossCouple, 0.0f, 0.35f);
    const float selfScale = 1.0f - cross;
    const float crossScale = cross;
    const float dampingAlpha = 0.92f - current_.damping * 0.86f;
    const float trim = dbToGain(current_.trimDb);
    float wet = 0.0f;

    for (int voiceIndex = 0; voiceIndex < voiceCount; ++voiceIndex)
    {
        auto& voice = state.voices[static_cast<std::size_t>(voiceIndex)];
        smooth(currentDelays_[static_cast<std::size_t>(ch)][static_cast<std::size_t>(voiceIndex)],
               targetDelays_[static_cast<std::size_t>(ch)][static_cast<std::size_t>(voiceIndex)]);
        voice.delaySamples = clamp(currentDelays_[static_cast<std::size_t>(ch)][static_cast<std::size_t>(voiceIndex)],
                                   2.0f,
                                   static_cast<float>(delayLineLength_ - 2));

        const float delayed = readFractional(voice, voice.delaySamples);
        const float coupled = channelStates_[other].lastWet;
        const float feedbackInput = input + feedback * polarity * (selfScale * delayed + crossScale * coupled);
        voice.damped += (feedbackInput - voice.damped) * dampingAlpha;
        const float dampingLoss = 1.0f - current_.damping * 0.7f;
        const float stored = clamp(sanitize(voice.damped * dampingLoss), -4.0f, 4.0f);
        voice.delay[static_cast<std::size_t>(voice.writeIndex)] = stored;
        voice.writeIndex = (voice.writeIndex + 1) % static_cast<int>(delayLineLength_);
        wet += delayed;
    }

    wet = clamp(wet * 0.25f, -4.0f, 4.0f);
    state.lastWet = wet;
    const float mixed = input + (wet - input) * clamp(current_.mix, 0.0f, 1.0f);
    return clamp(sanitize(mixed * trim), -2.0f, 2.0f);
}

float NailCombDSP::currentDelaySamples(int channel, int voice) const noexcept
{
    const int ch = channel <= 0 ? 0 : 1;
    const int v = voice < 0 ? 0 : (voice >= voiceCount ? voiceCount - 1 : voice);
    return currentDelays_[static_cast<std::size_t>(ch)][static_cast<std::size_t>(v)];
}

float NailCombDSP::currentFeedbackMagnitude() const noexcept
{
    return clamp(target_.feedback, 0.0f, maximumFeedback);
}

float NailCombDSP::sanitize(float value) noexcept
{
    return std::isfinite(value) ? value : 0.0f;
}

float NailCombDSP::clamp(float value, float lo, float hi) noexcept
{
    return value < lo ? lo : (value > hi ? hi : value);
}

float NailCombDSP::dbToGain(float valueDb) noexcept
{
    return std::pow(10.0f, valueDb / 20.0f);
}

float NailCombDSP::centsToRatio(float cents) noexcept
{
    return std::pow(2.0f, cents / 1200.0f);
}

float NailCombDSP::readFractional(const Voice& voice, float delaySamples) noexcept
{
    const auto length = static_cast<int>(voice.delay.size());
    if (length <= 2)
        return 0.0f;

    float read = static_cast<float>(voice.writeIndex) - delaySamples;
    while (read < 0.0f)
        read += static_cast<float>(length);
    while (read >= static_cast<float>(length))
        read -= static_cast<float>(length);

    const int i0 = static_cast<int>(read);
    const int i1 = (i0 + 1) % length;
    const float frac = read - static_cast<float>(i0);
    return sanitize(voice.delay[static_cast<std::size_t>(i0)] * (1.0f - frac)
                    + voice.delay[static_cast<std::size_t>(i1)] * frac);
}

void NailCombDSP::smooth(float& current, float target) noexcept
{
    constexpr float coefficient = 0.004f;
    current += (target - current) * coefficient;
}

void NailCombDSP::updateDerivedTargets() noexcept
{
    constexpr std::array<float, voiceCount> voiceOffsets { -1.0f, -0.33f, 0.37f, 1.0f };
    constexpr std::array<float, voiceCount> smearOffsets { 0.0f, 19.0f, -31.0f, 47.0f };
    const float baseFrequency = target_.frequencyHz * centsToRatio(target_.fineCents);
    const float spreadCents = target_.voiceSpread * 180.0f;

    for (int ch = 0; ch < 2; ++ch)
    {
        const float channelDetune = ch == 0 ? -target_.stereoDetuneCents : target_.stereoDetuneCents;
        for (int voice = 0; voice < voiceCount; ++voice)
        {
            const float voiceCents = voiceOffsets[static_cast<std::size_t>(voice)] * spreadCents
                + smearOffsets[static_cast<std::size_t>(voice)] * target_.smear
                + channelDetune;
            const float voiceFrequency = clamp(baseFrequency * centsToRatio(voiceCents), minimumFrequencyHz, maximumFrequencyHz);
            const float delay = static_cast<float>(sampleRate_ / voiceFrequency);
            targetDelays_[static_cast<std::size_t>(ch)][static_cast<std::size_t>(voice)] =
                clamp(delay, 2.0f, static_cast<float>(delayLineLength_ - 2));
        }
    }
}

void NailCombDSP::resizeDelayLines(std::size_t delaySamples)
{
    delayLineLength_ = std::max<std::size_t>(delaySamples, 8);
    for (auto& channel : channelStates_)
        for (auto& voice : channel.voices)
            voice.delay.assign(delayLineLength_, 0.0f);
}
} // namespace nailcomb::dsp
