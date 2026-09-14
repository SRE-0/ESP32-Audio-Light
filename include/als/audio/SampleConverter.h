#pragma once

#include <cstdint>
#include <vector>

namespace als {

enum class AudioSampleFormat {
    Float32,
    Int16,
    Int24,
    Int32,
    Unsupported
};

class SampleConverter {
public:
    static void toMono(
        const void* input,
        std::uint32_t frameCount,
        std::uint32_t channelCount,
        AudioSampleFormat format,
        std::vector<float>& output);
};

}  // namespace als
