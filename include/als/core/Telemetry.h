#pragma once

#include "als/audio/FrameAnalysis.h"
#include "als/core/Pixel.h"

#include <cstdint>
#include <mutex>

namespace als {

struct TelemetrySnapshot {
    float rms = 0.0F;
    bool beatDetected = false;
    Pixel color{};
    std::uint64_t processedFrames = 0;
};

class Telemetry {
public:
    void publish(const FrameAnalysis& analysis, const Pixel& color);
    TelemetrySnapshot snapshot() const;

private:
    mutable std::mutex mutex_;
    TelemetrySnapshot value_;
};

}  // namespace als
