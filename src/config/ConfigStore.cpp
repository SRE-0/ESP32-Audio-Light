#include "als/config/ConfigStore.h"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <string>
#include <system_error>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#endif

namespace als {
namespace {

std::string trim(const std::string& value) {
    const std::size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const std::size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

bool parseInt(const std::string& value, int& result) {
    try {
        std::size_t parsed = 0;
        const int candidate = std::stoi(value, &parsed);
        if (parsed != value.size()) {
            return false;
        }
        result = candidate;
        return true;
    } catch (...) {
        return false;
    }
}

bool parseFloat(const std::string& value, float& result) {
    try {
        std::size_t parsed = 0;
        const float candidate = std::stof(value, &parsed);
        if (parsed != value.size() || !std::isfinite(candidate)) {
            return false;
        }
        result = candidate;
        return true;
    } catch (...) {
        return false;
    }
}

bool isValidIpv4(const std::string& value) {
    std::size_t start = 0;
    for (int part = 0; part < 4; ++part) {
        const std::size_t end = value.find('.', start);
        if ((part < 3 && end == std::string::npos) ||
            (part == 3 && end != std::string::npos)) {
            return false;
        }
        const std::string segment = value.substr(
            start, end == std::string::npos ? end : end - start);
        if (segment.empty() || segment.size() > 3) {
            return false;
        }
        int number = 0;
        if (!parseInt(segment, number) || number < 0 || number > 255) {
            return false;
        }
        start = end + 1;
    }
    return true;
}

}  // namespace

ConfigStore::ConfigStore(std::filesystem::path path)
    : path_(std::move(path)) {}

Config ConfigStore::load(const Config& defaults) const {
    Config config = defaults;
    std::ifstream input(path_);
    if (!input) {
        return config;
    }

    std::string line;
    while (std::getline(input, line)) {
        line = trim(line);
        if (line.empty() || line.front() == '#' || line.front() == ';') {
            continue;
        }

        const std::size_t separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }
        const std::string key = trim(line.substr(0, separator));
        const std::string value = trim(line.substr(separator + 1));

        int integer = 0;
        float decimal = 0.0F;
        if (key == "ip" && isValidIpv4(value)) {
            config.ip = value;
        } else if (key == "port" && parseInt(value, integer) &&
                   integer >= 1 && integer <= 65535) {
            config.port = integer;
        } else if (key == "brightness" && parseFloat(value, decimal) &&
                   decimal >= 0.0F && decimal <= 10.0F) {
            config.brightness = decimal;
        } else if (key == "effect" && parseInt(value, integer) &&
                   integer >= static_cast<int>(EffectType::AmplitudeGradient) &&
                   integer <= static_cast<int>(EffectType::RainbowPulse)) {
            config.effect = static_cast<EffectType>(integer);
        } else if (key == "beat_sensitivity" &&
                   parseFloat(value, decimal) &&
                   decimal >= 1.0F && decimal <= 3.0F) {
            config.beatSensitivity = decimal;
        } else if (key == "decay_rate" && parseFloat(value, decimal) &&
                   decimal >= 0.8F && decimal <= 0.99F) {
            config.decayRate = decimal;
        }
    }
    return config;
}

bool ConfigStore::save(const Config& config) const {
    std::error_code error;
    const std::filesystem::path directory = path_.parent_path();
    if (!directory.empty()) {
        std::filesystem::create_directories(directory, error);
        if (error) {
            return false;
        }
    }

    std::filesystem::path temporary = path_;
    temporary += ".tmp";
    {
        std::ofstream output(temporary, std::ios::trunc);
        if (!output) {
            return false;
        }
        output << "# Audio Light Sync settings\n"
               << "ip=" << config.ip << '\n'
               << "port=" << config.port << '\n'
               << std::setprecision(9)
               << "brightness=" << config.brightness << '\n'
               << "effect=" << static_cast<int>(config.effect) << '\n'
               << "beat_sensitivity=" << config.beatSensitivity << '\n'
               << "decay_rate=" << config.decayRate << '\n';
        output.flush();
        if (!output) {
            return false;
        }
    }

#ifdef _WIN32
    const bool replaced = MoveFileExW(
        temporary.c_str(), path_.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    std::filesystem::rename(temporary, path_, error);
    const bool replaced = !error;
#endif
    if (!replaced) {
        std::filesystem::remove(temporary, error);
        return false;
    }
    return true;
}

const std::filesystem::path& ConfigStore::path() const {
    return path_;
}

std::filesystem::path ConfigStore::defaultPath() {
#ifdef _WIN32
    if (const char* appData = std::getenv("APPDATA")) {
        return std::filesystem::path(appData) /
               "AudioLightSync" / "settings.ini";
    }
#else
    if (const char* xdgConfig = std::getenv("XDG_CONFIG_HOME")) {
        return std::filesystem::path(xdgConfig) /
               "audio-light-sync" / "settings.ini";
    }
    if (const char* userHome = std::getenv("HOME")) {
        return std::filesystem::path(userHome) /
               ".config" / "audio-light-sync" / "settings.ini";
    }
#endif
    return std::filesystem::current_path() / "settings.ini";
}

}  // namespace als
