#include "als/effects/EffectEngine.h"

#include "als/effects/ColorUtils.h"

#include <algorithm>
#include <cmath>

namespace als {
namespace {

std::uint8_t colorComponent(float value) {
    return static_cast<std::uint8_t>(
        std::clamp(static_cast<int>(value), 0, 255));
}

}  // namespace

Pixel EffectEngine::render(const FrameAnalysis& analysis, const Config& config) {
    switch (config.effect) {
        case EffectType::AmplitudeGradient:
            return amplitudeGradient(analysis, config.brightness);
        case EffectType::BeatPulse:
            return beatPulse(analysis, config);
        case EffectType::FrequencySpectrum:
            return frequencySpectrum(analysis, config.brightness);
        case EffectType::EnergyWave:
            return energyWave(analysis, config);
        case EffectType::RainbowPulse:
            return rainbowPulse(analysis, config.brightness);
    }
    return {};
}

Pixel EffectEngine::amplitudeGradient(
    const FrameAnalysis& analysis, float brightness) const {
    return {
        colorComponent(analysis.rms * 255.0F * brightness),
        colorComponent((analysis.beatDetected ? 255.0F : analysis.rms * 128.0F) *
                       brightness),
        colorComponent((1.0F - analysis.rms) * 255.0F * brightness),
        1
    };
}

Pixel EffectEngine::beatPulse(
    const FrameAnalysis& analysis, const Config& config) {
    if (analysis.beatDetected) {
        beatPulseLevel_ = 1.0F;
    }
    beatPulseLevel_ *= config.decayRate;
    return {
        colorComponent(255.0F * config.brightness * beatPulseLevel_),
        colorComponent(100.0F * config.brightness * beatPulseLevel_),
        colorComponent(50.0F * config.brightness * beatPulseLevel_),
        1
    };
}

Pixel EffectEngine::frequencySpectrum(
    const FrameAnalysis& analysis, float brightness) const {
    return {
        colorComponent(analysis.spectrum.bass * 255.0F * brightness),
        colorComponent(analysis.spectrum.mid * 255.0F * brightness),
        colorComponent(analysis.spectrum.treble * 255.0F * brightness),
        1
    };
}

Pixel EffectEngine::energyWave(
    const FrameAnalysis& analysis, const Config& config) {
    if (analysis.beatDetected) {
        energyLevel_ += 0.5F;
    }
    energyLevel_ = std::min(1.0F, energyLevel_ * config.decayRate);
    const float baseHue = std::fmod(energyLevel_ * 360.0F, 360.0F);
    return hsvToRgb(baseHue, 1.0F, analysis.rms * config.brightness);
}

Pixel EffectEngine::rainbowPulse(
    const FrameAnalysis& analysis, float brightness) {
    hue_ = std::fmod(hue_ + analysis.rms * 2.0F, 360.0F);
    const float saturation = analysis.beatDetected ? 1.0F : 0.8F;
    const float value = analysis.beatDetected
        ? 1.0F
        : std::min(1.0F, analysis.rms * 2.0F);
    return hsvToRgb(hue_, saturation, value * brightness);
}

}  // namespace als
