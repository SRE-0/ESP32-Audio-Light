#pragma once

#include "als/audio/SignalAnalyzer.h"
#include "als/config/RuntimeConfig.h"
#include "als/core/Pixel.h"
#include "als/core/Telemetry.h"
#include "als/effects/EffectEngine.h"
#include "als/network/IColorSender.h"

#include <cstdint>
#include <vector>

namespace als {

class AudioProcessor {
public:
    AudioProcessor(
        RuntimeConfig& config,
        IColorSender& colorSender,
        Telemetry& telemetry);

    bool ready() const;
    void process(const float* samples, std::uint32_t count);

private:
    void processFullBuffer();

    RuntimeConfig& config_;
    IColorSender& colorSender_;
    Telemetry& telemetry_;
    SignalAnalyzer analyzer_;
    EffectEngine effectEngine_;
    std::vector<float> sampleBuffer_;
    std::size_t writePosition_ = 0;
    Pixel lastSent_{0, 0, 0, 0};
    std::uint64_t frameCount_ = 0;
};

}  // namespace als
