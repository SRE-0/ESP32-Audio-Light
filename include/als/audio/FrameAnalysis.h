#pragma once

namespace als {

struct Spectrum {
    float bass = 0.0F;
    float mid = 0.0F;
    float treble = 0.0F;
};

struct FrameAnalysis {
    float rms = 0.0F;
    bool beatDetected = false;
    Spectrum spectrum;
};

}  // namespace als
