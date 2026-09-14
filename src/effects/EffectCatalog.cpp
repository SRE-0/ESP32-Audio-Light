#include "als/effects/EffectCatalog.h"

namespace als {

const std::vector<EffectOption>& availableEffects() {
    static const std::vector<EffectOption> effects = {
        {1, "Amplitude Gradient", EffectType::AmplitudeGradient},
        {2, "Beat Pulse", EffectType::BeatPulse},
        {3, "Frequency Spectrum", EffectType::FrequencySpectrum},
        {4, "Energy Wave", EffectType::EnergyWave},
        {5, "Rainbow Pulse", EffectType::RainbowPulse},
    };
    return effects;
}

std::string_view effectName(EffectType effect) {
    for (const auto& option : availableEffects()) {
        if (option.type == effect) {
            return option.name;
        }
    }
    return "Unknown";
}

const EffectOption* findEffect(int id) {
    for (const auto& option : availableEffects()) {
        if (option.id == id) {
            return &option;
        }
    }
    return nullptr;
}

}  // namespace als
