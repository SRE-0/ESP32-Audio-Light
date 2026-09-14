#include "als/audio/SignalAnalyzer.h"

#include "als/core/Constants.h"

#include <algorithm>
#include <cmath>

namespace als {
namespace {

constexpr float kPi = 3.14159265358979323846F;

float magnitude(const fftwf_complex& value) {
    return std::sqrt(value[0] * value[0] + value[1] * value[1]);
}

}  // namespace

SignalAnalyzer::SignalAnalyzer(bool spectrumEnabled)
    : spectrumEnabled_(spectrumEnabled) {
    initializeWindow();
    if (spectrumEnabled_) {
        initializeFft();
    }
}

SignalAnalyzer::~SignalAnalyzer() {
    if (fftPlan_) {
        fftwf_destroy_plan(fftPlan_);
    }
    if (fftInput_) {
        fftwf_free(fftInput_);
    }
    if (fftOutput_) {
        fftwf_free(fftOutput_);
    }
}

bool SignalAnalyzer::ready() const {
    return !spectrumEnabled_ || fftPlan_ != nullptr;
}

void SignalAnalyzer::initializeWindow() {
    hannWindow_.resize(kBufferSize);
    for (std::uint32_t index = 0; index < kBufferSize; ++index) {
        hannWindow_[index] = 0.5F * (
            1.0F - std::cos(2.0F * kPi * static_cast<float>(index) /
                            static_cast<float>(kBufferSize - 1)));
    }
}

bool SignalAnalyzer::initializeFft() {
    fftInput_ = static_cast<fftwf_complex*>(
        fftwf_malloc(sizeof(fftwf_complex) * kFftSize));
    fftOutput_ = static_cast<fftwf_complex*>(
        fftwf_malloc(sizeof(fftwf_complex) * kFftSize));
    if (!fftInput_ || !fftOutput_) {
        return false;
    }
    fftPlan_ = fftwf_plan_dft_1d(
        static_cast<int>(kFftSize), fftInput_, fftOutput_,
        FFTW_FORWARD, FFTW_ESTIMATE);
    return fftPlan_ != nullptr;
}

FrameAnalysis SignalAnalyzer::analyze(const float* samples, const Config& config) {
    float sumSquares = 0.0F;
    for (std::uint32_t index = 0; index < kBufferSize; ++index) {
        const float sample = samples[index] * hannWindow_[index];
        sumSquares += sample * sample;
    }

    FrameAnalysis result;
    result.rms = std::sqrt(sumSquares / static_cast<float>(kBufferSize));
    result.beatDetected =
        result.rms > movingAverage_ * config.beatSensitivity &&
        result.rms > kRmsThreshold;
    movingAverage_ =
        config.decayRate * movingAverage_ + (1.0F - config.decayRate) * result.rms;

    if (spectrumEnabled_ && fftPlan_) {
        result.spectrum = analyzeSpectrum(samples);
    }
    return result;
}

Spectrum SignalAnalyzer::analyzeSpectrum(const float* samples) {
    for (std::uint32_t index = 0; index < kFftSize; ++index) {
        fftInput_[index][0] = samples[index] * hannWindow_[index];
        fftInput_[index][1] = 0.0F;
    }
    fftwf_execute(fftPlan_);

    Spectrum result;
    for (std::uint32_t index = 2; index < 40; ++index) {
        result.bass += magnitude(fftOutput_[index]);
    }
    for (std::uint32_t index = 40; index < 200; ++index) {
        result.mid += magnitude(fftOutput_[index]);
    }
    for (std::uint32_t index = 200; index < 500; ++index) {
        result.treble += magnitude(fftOutput_[index]);
    }
    result.bass = std::min(1.0F, result.bass / 1000.0F);
    result.mid = std::min(1.0F, result.mid / 500.0F);
    result.treble = std::min(1.0F, result.treble / 200.0F);
    return result;
}

}  // namespace als
