#include "als/core/Telemetry.h"

namespace als {

void Telemetry::publish(const FrameAnalysis& analysis, const Pixel& color) {
    const std::lock_guard<std::mutex> lock(mutex_);
    value_.rms = analysis.rms;
    value_.beatDetected = analysis.beatDetected;
    value_.color = color;
    ++value_.processedFrames;
}

TelemetrySnapshot Telemetry::snapshot() const {
    const std::lock_guard<std::mutex> lock(mutex_);
    return value_;
}

}  // namespace als
