#pragma once

#include "als/config/Config.h"

#include <filesystem>

namespace als {

class ConfigStore {
public:
    explicit ConfigStore(std::filesystem::path path = defaultPath());

    Config load(const Config& defaults = {}) const;
    bool save(const Config& config) const;

    const std::filesystem::path& path() const;
    static std::filesystem::path defaultPath();

private:
    std::filesystem::path path_;
};

}  // namespace als
