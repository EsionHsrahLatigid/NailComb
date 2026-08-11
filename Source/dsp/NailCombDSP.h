#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace nailcomb::dsp
{
struct NailCombParameters
{
    float frequencyHz { 110.0f };
    float fineCents { 0.0f };
    float feedback { 0.65f };
    float polarity { 1.0f };
    float damping { 0.35f };
    float smear { 0.25f };
    float stereoDetuneCents { 7.0f };
    float crossCouple { 0.12f };
    float voiceSpread { 0.4f };
    float mix { 0.6f };
    float trimDb { -3.0f };
};

class NailCombDSP
{
public:
    static constexpr int voiceCount = 4;
    static constexpr float minimumFrequencyHz = 20.0f;
    static constexpr float maximumFrequencyHz = 5000.0f;
    static constexpr float maximumFeedback = 0.94f;

    void prepare(double sampleRate, int maxBlockSize, int channels);
    void reset() noexcept;
    void setTargets(const NailCombParameters& parameters) noexcept;
    float processSample(float input, int channel) noexcept;

    int preparedChannels() const noexcept { return channels_; }
    double sampleRate() const noexcept { return sampleRate_; }
    float currentDelaySamples(int channel, int voice) const noexcept;
    float currentFeedbackMagnitude() const noexcept;

private:
    struct Voice
    {
        std::vector<float> delay;
        int writeIndex { 0 };
        float damped { 0.0f };
        float delaySamples { 256.0f };
    };

    struct ChannelState
    {
        std::array<Voice, voiceCount> voices;
        float lastWet { 0.0f };
    };

    static float sanitize(float value) noexcept;
    static float clamp(float value, float lo, float hi) noexcept;
    static float dbToGain(float valueDb) noexcept;
    static float centsToRatio(float cents) noexcept;
    static float readFractional(const Voice& voice, float delaySamples) noexcept;
    static void smooth(float& current, float target) noexcept;

    void updateDerivedTargets() noexcept;
    void resizeDelayLines(std::size_t delaySamples);

    double sampleRate_ { 44100.0 };
    int channels_ { 0 };
    std::size_t delayLineLength_ { 4096 };
    std::array<ChannelState, 2> channelStates_;

    NailCombParameters target_;
    NailCombParameters current_;
    std::array<std::array<float, voiceCount>, 2> targetDelays_ {};
    std::array<std::array<float, voiceCount>, 2> currentDelays_ {};
};
} // namespace nailcomb::dsp
