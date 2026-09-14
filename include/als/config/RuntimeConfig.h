#pragma once

#include "als/config/Config.h"

#include <mutex>

namespace als {

class RuntimeConfig {
public:
    explicit RuntimeConfig(Config initial = {});

    Config snapshot() const;
    void update(const Config& config);

private:
    mutable std::mutex mutex_;
    Config config_;
};

}  // namespace als
