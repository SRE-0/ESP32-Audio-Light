#include "als/config/RuntimeConfig.h"

#include <utility>

namespace als {

RuntimeConfig::RuntimeConfig(Config initial)
    : config_(std::move(initial)) {}

Config RuntimeConfig::snapshot() const {
    const std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

void RuntimeConfig::update(const Config& config) {
    const std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
}

}  // namespace als
