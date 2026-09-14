#include "als/application/Application.h"

#include "als/audio/AudioProcessor.h"
#include "als/audio/BackendFactory.h"

#include <future>
#include <utility>

namespace als {

Application::Application()
    : configStore_(), config_(configStore_.load()) {}

Application::Application(Config initialConfig)
    : configStore_(), config_(std::move(initialConfig)) {}

Application::~Application() {
    stop();
}

bool Application::start() {
    if (captureThread_.joinable()) {
        return running();
    }

    std::promise<bool> initialized;
    auto result = initialized.get_future();
    captureThread_ = std::thread(
        &Application::captureLoop, this, std::move(initialized));
    const bool started = result.get();
    if (!started && captureThread_.joinable()) {
        captureThread_.join();
    }
    return started;
}

void Application::captureLoop(std::promise<bool> initialized) {
    const Config initialConfig = config_.snapshot();
    if (!colorSender_.initialize(initialConfig)) {
        setError("No se pudo inicializar la salida UDP.");
        initialized.set_value(false);
        return;
    }

    audioBackend_ = createAudioBackend();
    if (!audioBackend_ || !audioBackend_->initialize()) {
        setError("No se pudo inicializar el dispositivo de audio.");
        initialized.set_value(false);
        return;
    }

    audioProcessor_ = std::make_unique<AudioProcessor>(
        config_, colorSender_, telemetry_);
    if (!audioProcessor_->ready()) {
        setError("No se pudo inicializar el analizador FFT.");
        audioBackend_->shutdown();
        initialized.set_value(false);
        return;
    }

    sampleRate_ = audioBackend_->actualSampleRate();
    channelCount_ = audioBackend_->actualChannels();
    running_ = true;
    initialized.set_value(true);

    audioBackend_->startCapture(
        [this](const float* samples, std::uint32_t count) {
            if (!paused_) {
                audioProcessor_->process(samples, count);
            }
        });

    running_ = false;
    audioBackend_->shutdown();
}

void Application::stop() {
    if (audioBackend_) {
        audioBackend_->stop();
    }
    if (captureThread_.joinable()) {
        captureThread_.join();
    }
    running_ = false;
    audioProcessor_.reset();
    audioBackend_.reset();
    colorSender_.close();
}

Config Application::config() const {
    return config_.snapshot();
}

bool Application::updateConfig(const Config& updated) {
    const Config previous = config_.snapshot();
    if ((updated.ip != previous.ip || updated.port != previous.port) &&
        !colorSender_.initialize(updated)) {
        setError("No se pudo aplicar el nuevo destino UDP.");
        return false;
    }
    if (!configStore_.save(updated)) {
        if (updated.ip != previous.ip || updated.port != previous.port) {
            colorSender_.initialize(previous);
        }
        setError("No se pudo guardar la configuracion en el disco.");
        return false;
    }
    config_.update(updated);
    setError({});
    return true;
}

bool Application::running() const {
    return running_;
}

bool Application::paused() const {
    return paused_;
}

void Application::setPaused(bool paused) {
    paused_ = paused;
}

std::uint32_t Application::sampleRate() const {
    return sampleRate_;
}

std::uint32_t Application::channelCount() const {
    return channelCount_;
}

TelemetrySnapshot Application::telemetry() const {
    return telemetry_.snapshot();
}

std::string Application::lastError() const {
    const std::lock_guard<std::mutex> lock(errorMutex_);
    return lastError_;
}

void Application::setError(std::string message) {
    const std::lock_guard<std::mutex> lock(errorMutex_);
    lastError_ = std::move(message);
}

}  // namespace als
