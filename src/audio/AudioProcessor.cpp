#include "als/audio/AudioProcessor.h"

#include "als/core/Constants.h"
#include "als/effects/ColorUtils.h"
#include "als/effects/EffectCatalog.h"

#include <algorithm>
#include <cstring>

namespace als {

AudioProcessor::AudioProcessor(
    RuntimeConfig& config,
    IColorSender& colorSender,
    Telemetry& telemetry)
    : config_(config),
      colorSender_(colorSender),
      telemetry_(telemetry),
      analyzer_(true),
      sampleBuffer_(kBufferSize, 0.0F) {}

bool AudioProcessor::ready() const {
    return analyzer_.ready();
}

void AudioProcessor::process(const float* samples, std::uint32_t count) {
    if (!samples || count == 0) {
        return;
    }

    std::size_t sourceOffset = 0;
    while (sourceOffset < count) {
        const std::size_t available = kBufferSize - writePosition_;
        const std::size_t copyCount =
            std::min<std::size_t>(count - sourceOffset, available);
        std::memcpy(
            sampleBuffer_.data() + writePosition_,
            samples + sourceOffset,
            copyCount * sizeof(float));
        writePosition_ += copyCount;
        sourceOffset += copyCount;

        if (writePosition_ == kBufferSize) {
            processFullBuffer();
            writePosition_ = 0;
        }
    }
}

void AudioProcessor::processFullBuffer() {
    ++frameCount_;
    const Config config = config_.snapshot();
    const FrameAnalysis analysis = analyzer_.analyze(sampleBuffer_.data(), config);
    const Pixel target = effectEngine_.render(analysis, config);
    const float alpha = analysis.beatDetected ? 0.6F : 0.12F;
    const Pixel smoothed = smoothColor(lastSent_, target, alpha);

    if (smoothed != lastSent_) {
        colorSender_.send(smoothed);
        lastSent_ = smoothed;
    }
    telemetry_.publish(analysis, smoothed);
}

}  // namespace als
