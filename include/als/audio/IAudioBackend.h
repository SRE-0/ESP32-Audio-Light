#pragma once

#include <cstdint>
#include <functional>

namespace als {

using AudioCallback = std::function<void(const float* samples, std::uint32_t count)>;

class IAudioBackend {
public:
    virtual ~IAudioBackend() = default;
    virtual bool initialize() = 0;
    virtual void startCapture(AudioCallback callback) = 0;
    virtual void stop() = 0;
    virtual void shutdown() = 0;
    virtual std::uint32_t actualSampleRate() const = 0;
    virtual std::uint32_t actualChannels() const = 0;
};

}  // namespace als
