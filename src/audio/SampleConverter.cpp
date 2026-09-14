#include "als/audio/SampleConverter.h"

#include <algorithm>
#include <cstdint>

namespace als {
namespace {

template <typename Sample, typename Converter>
void downmix(
    const Sample* input,
    std::uint32_t frameCount,
    std::uint32_t channelCount,
    std::vector<float>& output,
    Converter converter) {
    for (std::uint32_t frame = 0; frame < frameCount; ++frame) {
        float sum = 0.0F;
        for (std::uint32_t channel = 0; channel < channelCount; ++channel) {
            sum += converter(input[frame * channelCount + channel]);
        }
        output[frame] = sum / static_cast<float>(channelCount);
    }
}

void downmixInt24(
    const std::uint8_t* input,
    std::uint32_t frameCount,
    std::uint32_t channelCount,
    std::vector<float>& output) {
    for (std::uint32_t frame = 0; frame < frameCount; ++frame) {
        float sum = 0.0F;
        for (std::uint32_t channel = 0; channel < channelCount; ++channel) {
            const auto* sample = input + (frame * channelCount + channel) * 3;
            std::int32_t value =
                (static_cast<std::int32_t>(sample[2]) << 16) |
                (static_cast<std::int32_t>(sample[1]) << 8) |
                static_cast<std::int32_t>(sample[0]);
            if (value & 0x800000) {
                value |= static_cast<std::int32_t>(0xFF000000);
            }
            sum += static_cast<float>(value) / 8388608.0F;
        }
        output[frame] = sum / static_cast<float>(channelCount);
    }
}

}  // namespace

void SampleConverter::toMono(
    const void* input,
    std::uint32_t frameCount,
    std::uint32_t channelCount,
    AudioSampleFormat format,
    std::vector<float>& output) {
    output.resize(frameCount);
    if (!input || channelCount == 0) {
        std::fill(output.begin(), output.end(), 0.0F);
        return;
    }

    switch (format) {
        case AudioSampleFormat::Float32:
            downmix(
                static_cast<const float*>(input), frameCount, channelCount, output,
                [](float value) { return value; });
            break;
        case AudioSampleFormat::Int16:
            downmix(
                static_cast<const std::int16_t*>(input),
                frameCount, channelCount, output,
                [](std::int16_t value) {
                    return static_cast<float>(value) / 32768.0F;
                });
            break;
        case AudioSampleFormat::Int24:
            downmixInt24(
                static_cast<const std::uint8_t*>(input),
                frameCount, channelCount, output);
            break;
        case AudioSampleFormat::Int32:
            downmix(
                static_cast<const std::int32_t*>(input),
                frameCount, channelCount, output,
                [](std::int32_t value) {
                    return static_cast<float>(value) / 2147483648.0F;
                });
            break;
        case AudioSampleFormat::Unsupported:
            std::fill(output.begin(), output.end(), 0.0F);
            break;
    }
}

}  // namespace als
