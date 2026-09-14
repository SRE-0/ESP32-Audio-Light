#pragma once

#include <cstdint>

namespace als {

inline constexpr std::uint32_t kSampleRate = 44100;
inline constexpr std::uint32_t kChannels = 1;
inline constexpr std::uint32_t kBufferSize = 1024;
inline constexpr std::uint32_t kFftSize = 1024;
inline constexpr float kRmsThreshold = 0.15F;

}  // namespace als
