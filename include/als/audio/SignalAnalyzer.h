#pragma once

#include "als/audio/FrameAnalysis.h"
#include "als/config/Config.h"

#include <fftw3.h>

#include <cstddef>
#include <vector>

namespace als {

class SignalAnalyzer {
public:
    explicit SignalAnalyzer(bool spectrumEnabled);
    ~SignalAnalyzer();

    SignalAnalyzer(const SignalAnalyzer&) = delete;
    SignalAnalyzer& operator=(const SignalAnalyzer&) = delete;

    bool ready() const;
    FrameAnalysis analyze(const float* samples, const Config& config);

private:
    void initializeWindow();
    bool initializeFft();
    Spectrum analyzeSpectrum(const float* samples);

    std::vector<float> hannWindow_;
    float movingAverage_ = 0.0F;
    bool spectrumEnabled_ = false;
    fftwf_complex* fftInput_ = nullptr;
    fftwf_complex* fftOutput_ = nullptr;
    fftwf_plan fftPlan_ = nullptr;
};

}  // namespace als
