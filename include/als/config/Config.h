#pragma once

#include <string>

namespace als {

enum class EffectType {
    AmplitudeGradient,
    BeatPulse,
    FrequencySpectrum,
    EnergyWave,
    RainbowPulse
};

struct Config {
    std::string ip = "192.168.1.200";
    int port = 8888;
    float brightness = 5.0F;
    EffectType effect = EffectType::EnergyWave;
    float beatSensitivity = 1.5F;
    float decayRate = 0.8F;
};

}  // namespace als
