#pragma once

#include "als/audio/FrameAnalysis.h"
#include "als/config/Config.h"
#include "als/core/Pixel.h"

namespace als {

class EffectEngine {
public:
    Pixel render(const FrameAnalysis& analysis, const Config& config);

private:
    Pixel amplitudeGradient(const FrameAnalysis& analysis, float brightness) const;
    Pixel beatPulse(const FrameAnalysis& analysis, const Config& config);
    Pixel frequencySpectrum(const FrameAnalysis& analysis, float brightness) const;
    Pixel energyWave(const FrameAnalysis& analysis, const Config& config);
    Pixel rainbowPulse(const FrameAnalysis& analysis, float brightness);

    float beatPulseLevel_ = 0.0F;
    float energyLevel_ = 0.0F;
    float hue_ = 0.0F;
};

}  // namespace als
