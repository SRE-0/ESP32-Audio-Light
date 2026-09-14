#include "als/audio/SampleConverter.h"
#include "als/audio/FrameAnalysis.h"
#include "als/config/Config.h"
#include "als/config/ConfigStore.h"
#include "als/config/RuntimeConfig.h"
#include "als/core/Telemetry.h"
#include "als/effects/ColorUtils.h"
#include "als/effects/EffectCatalog.h"
#include "als/effects/EffectEngine.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <vector>

namespace {

bool approximately(float actual, float expected, float tolerance = 0.0001F) {
    return std::fabs(actual - expected) <= tolerance;
}

void testEffectCatalog() {
    assert(als::availableEffects().size() == 5);
    assert(als::findEffect(3) != nullptr);
    assert(als::findEffect(3)->type == als::EffectType::FrequencySpectrum);
    assert(als::findEffect(99) == nullptr);
}

void testColorConversion() {
    const als::Pixel red = als::hsvToRgb(0.0F, 1.0F, 1.0F);
    assert(red.r == 255 && red.g == 0 && red.b == 0);
    const als::Pixel overdriven = als::hsvToRgb(0.0F, 1.0F, 5.0F);
    assert(overdriven.r == 255);

    const als::Pixel smoothed = als::smoothColor(
        {0, 0, 0, 1}, {100, 50, 200, 1}, 0.5F);
    assert(smoothed.r == 50 && smoothed.g == 25 && smoothed.b == 100);
}

void testStereoDownmix() {
    const std::vector<float> stereo = {1.0F, -1.0F, 0.5F, 0.25F};
    std::vector<float> mono;
    als::SampleConverter::toMono(
        stereo.data(), 2, 2, als::AudioSampleFormat::Float32, mono);
    assert(mono.size() == 2);
    assert(approximately(mono[0], 0.0F));
    assert(approximately(mono[1], 0.375F));
}

void testInt16Conversion() {
    const std::vector<std::int16_t> input = {32767, -32768};
    std::vector<float> output;
    als::SampleConverter::toMono(
        input.data(), 2, 1, als::AudioSampleFormat::Int16, output);
    assert(approximately(output[0], 32767.0F / 32768.0F));
    assert(approximately(output[1], -1.0F));
}

void testAmplitudeEffect() {
    als::EffectEngine engine;
    als::Config config;
    config.effect = als::EffectType::AmplitudeGradient;
    config.brightness = 1.0F;
    const als::Pixel pixel = engine.render({0.5F, false, {}}, config);
    assert(pixel.r == 127);
    assert(pixel.g == 64);
    assert(pixel.b == 127);
}

void testRuntimeConfig() {
    als::RuntimeConfig runtime;
    als::Config updated = runtime.snapshot();
    updated.brightness = 3.4F;
    updated.effect = als::EffectType::RainbowPulse;
    runtime.update(updated);

    const als::Config snapshot = runtime.snapshot();
    assert(approximately(snapshot.brightness, 3.4F));
    assert(snapshot.effect == als::EffectType::RainbowPulse);
}

void testConfigStore() {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "audio_light_sync_test.ini";
    std::error_code error;
    std::filesystem::remove(path, error);

    const als::ConfigStore store(path);
    als::Config saved;
    saved.ip = "10.0.0.42";
    saved.port = 4567;
    saved.brightness = 3.4F;
    saved.effect = als::EffectType::RainbowPulse;
    saved.beatSensitivity = 2.1F;
    saved.decayRate = 0.93F;

    assert(store.save(saved));
    const als::Config loaded = store.load();
    assert(loaded.ip == saved.ip);
    assert(loaded.port == saved.port);
    assert(approximately(loaded.brightness, saved.brightness));
    assert(loaded.effect == saved.effect);
    assert(approximately(loaded.beatSensitivity, saved.beatSensitivity));
    assert(approximately(loaded.decayRate, saved.decayRate));

    std::filesystem::remove(path, error);
}

void testTelemetry() {
    als::Telemetry telemetry;
    telemetry.publish({0.25F, true, {}}, {10, 20, 30, 1});
    const als::TelemetrySnapshot snapshot = telemetry.snapshot();
    assert(approximately(snapshot.rms, 0.25F));
    assert(snapshot.beatDetected);
    assert(snapshot.color.r == 10);
    assert(snapshot.processedFrames == 1);
}

}  // namespace

int main() {
    testEffectCatalog();
    testColorConversion();
    testStereoDownmix();
    testInt16Conversion();
    testAmplitudeEffect();
    testRuntimeConfig();
    testConfigStore();
    testTelemetry();
    std::cout << "All unit tests passed\n";
    return 0;
}
