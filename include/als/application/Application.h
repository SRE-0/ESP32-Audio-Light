#pragma once

#include "als/audio/IAudioBackend.h"
#include "als/config/ConfigStore.h"
#include "als/config/RuntimeConfig.h"
#include "als/core/Telemetry.h"
#include "als/network/UdpColorSender.h"

#include <atomic>
#include <cstdint>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace als {

class AudioProcessor;

class Application {
public:
    Application();
    explicit Application(Config initialConfig);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool start();
    void stop();

    Config config() const;
    bool updateConfig(const Config& config);

    bool running() const;
    bool paused() const;
    void setPaused(bool paused);

    std::uint32_t sampleRate() const;
    std::uint32_t channelCount() const;
    TelemetrySnapshot telemetry() const;
    std::string lastError() const;

private:
    void captureLoop(std::promise<bool> initialized);
    void setError(std::string message);

    ConfigStore configStore_;
    RuntimeConfig config_;
    UdpColorSender colorSender_;
    std::unique_ptr<IAudioBackend> audioBackend_;
    std::unique_ptr<AudioProcessor> audioProcessor_;
    Telemetry telemetry_;
    std::thread captureThread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};
    std::atomic<std::uint32_t> sampleRate_{0};
    std::atomic<std::uint32_t> channelCount_{0};
    mutable std::mutex errorMutex_;
    std::string lastError_;
};

}  // namespace als
