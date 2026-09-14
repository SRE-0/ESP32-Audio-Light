#include "als/ui/Dashboard.h"

#include "als/effects/EffectCatalog.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

namespace als {
namespace {

constexpr std::size_t kRowCount = 8;

std::string formatFloat(float value, int precision = 2) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

}  // namespace

Dashboard::Dashboard(
    Application& application,
    Terminal& terminal,
    std::ostream& output)
    : application_(application),
      terminal_(terminal),
      output_(output) {}

void Dashboard::run() {
    while (!exitRequested_ && application_.running()) {
        render();
        handle(terminal_.readKey(100));
    }
    application_.stop();
    terminal_.clear();
}

void Dashboard::render() {
    const Config config = application_.config();
    const TelemetrySnapshot telemetry = application_.telemetry();
    const bool paused = application_.paused();

    output_ << "\x1b[H"
            << "\x1b[1;36m"
            << "+------------------------------------------------------------+\n"
            << "|                    AUDIO LIGHT SYNC                        |\n"
            << "+------------------------------------------------------------+\x1b[0m\n"
            << " Estado: "
            << (paused ? "\x1b[33mPAUSADO" : "\x1b[32mACTIVO")
            << "\x1b[0m   Audio: " << application_.sampleRate() << " Hz / "
            << application_.channelCount() << " ch"
            << "   Frames: " << telemetry.processedFrames << "\n\n";

    output_ << selectedRow(
        0, "  Destino IP       [ EDITAR ]  " + config.ip) << '\n';
    output_ << selectedRow(
        1, "  Puerto           [ - ]  " + std::to_string(config.port) +
           "  [ + ]") << '\n';
    output_ << selectedRow(
        2, "  Brillo           [ - ]  " +
           formatFloat(config.brightness, 1) + "  [ + ]") << '\n';
    output_ << selectedRow(
        3, "  Efecto           [ < ]  " +
           std::string(effectName(config.effect)) + "  [ > ]") << '\n';
    output_ << selectedRow(
        4, "  Sensibilidad     [ - ]  " +
           formatFloat(config.beatSensitivity, 1) + "  [ + ]") << '\n';
    output_ << selectedRow(
        5, "  Decaimiento      [ - ]  " +
           formatFloat(config.decayRate, 2) + "  [ + ]") << "\n\n";
    output_ << selectedRow(
        6, paused ? "  [ REANUDAR ENVIO ]" : "  [ PAUSAR ENVIO ]") << '\n';
    output_ << selectedRow(7, "  [ SALIR ]") << "\n\n";

    output_ << " Nivel  " << levelBar(telemetry.rms * 5.0F, 32)
            << ' ' << formatFloat(telemetry.rms, 3)
            << (telemetry.beatDetected ? "  \x1b[1;33mBEAT!\x1b[0m" : "")
            << "\n Color  \x1b[48;2;"
            << static_cast<int>(telemetry.color.r) << ';'
            << static_cast<int>(telemetry.color.g) << ';'
            << static_cast<int>(telemetry.color.b)
            << "m      \x1b[0m  RGB("
            << static_cast<int>(telemetry.color.r) << ", "
            << static_cast<int>(telemetry.color.g) << ", "
            << static_cast<int>(telemetry.color.b) << ")\n\n";

    const std::string error = application_.lastError();
    if (!error.empty()) {
        output_ << " \x1b[31m" << error << "\x1b[0m\n";
    } else if (!notice_.empty()) {
        output_ << " \x1b[36m" << notice_ << "\x1b[0m\n";
    } else {
        output_ << " Flechas: navegar/cambiar | Enter: activar/editar"
                << " | P: pausa | Q: salir\n";
    }
    output_ << "\x1b[J" << std::flush;
}

void Dashboard::handle(const KeyEvent& event) {
    switch (event.key) {
        case Key::Up:
            selected_ = (selected_ + kRowCount - 1) % kRowCount;
            break;
        case Key::Down:
            selected_ = (selected_ + 1) % kRowCount;
            break;
        case Key::Left:
            adjustSelected(-1);
            break;
        case Key::Right:
            adjustSelected(1);
            break;
        case Key::Enter:
            activateSelected();
            break;
        case Key::Escape:
            exitRequested_ = true;
            break;
        case Key::Character:
            if (event.character == 'q' || event.character == 'Q') {
                exitRequested_ = true;
            } else if (event.character == 'p' || event.character == 'P') {
                application_.setPaused(!application_.paused());
            }
            break;
        case Key::None:
            break;
    }
}

void Dashboard::adjustSelected(int direction) {
    Config config = application_.config();
    switch (selected_) {
        case 1:
            config.port = std::clamp(config.port + direction, 1, 65535);
            break;
        case 2:
            config.brightness =
                std::clamp(config.brightness + direction * 0.1F, 0.0F, 10.0F);
            break;
        case 3: {
            const auto& effects = availableEffects();
            auto position = std::find_if(
                effects.begin(), effects.end(),
                [&config](const EffectOption& effect) {
                    return effect.type == config.effect;
                });
            std::size_t index = position == effects.end()
                ? 0
                : static_cast<std::size_t>(position - effects.begin());
            index = (index + effects.size() + direction) % effects.size();
            config.effect = effects[index].type;
            break;
        }
        case 4:
            config.beatSensitivity = std::clamp(
                config.beatSensitivity + direction * 0.1F, 1.0F, 3.0F);
            break;
        case 5:
            config.decayRate = std::clamp(
                config.decayRate + direction * 0.01F, 0.8F, 0.99F);
            break;
        default:
            return;
    }
    apply(config);
}

void Dashboard::activateSelected() {
    if (selected_ == 0) {
        editIp();
    } else if (selected_ == 1) {
        editPort();
    } else if (selected_ >= 2 && selected_ <= 5) {
        adjustSelected(1);
    } else if (selected_ == 6) {
        application_.setPaused(!application_.paused());
    } else if (selected_ == 7) {
        exitRequested_ = true;
    }
}

void Dashboard::editIp() {
    const Config current = application_.config();
    Config updated = current;
    updated.ip = terminal_.readLine(
        "Introduce la direccion IPv4 del ESP32", current.ip);
    if (!apply(updated)) {
        notice_ = "Direccion IP invalida; se conservo el valor anterior.";
    }
}

void Dashboard::editPort() {
    const Config current = application_.config();
    const std::string value = terminal_.readLine(
        "Introduce el puerto UDP (1-65535)", std::to_string(current.port));
    try {
        std::size_t parsed = 0;
        const int port = std::stoi(value, &parsed);
        if (parsed != value.size() || port < 1 || port > 65535) {
            throw std::out_of_range("port");
        }
        Config updated = current;
        updated.port = port;
        apply(updated);
    } catch (...) {
        notice_ = "Puerto invalido; se conservo el valor anterior.";
    }
}

bool Dashboard::apply(const Config& config) {
    if (!application_.updateConfig(config)) {
        return false;
    }
    notice_ = "Configuracion aplicada en tiempo real.";
    return true;
}

std::string Dashboard::selectedRow(
    std::size_t row, const std::string& text) const {
    if (row == selected_) {
        return "\x1b[1;7m> " + text + " \x1b[0m";
    }
    return "  " + text;
}

std::string Dashboard::levelBar(float value, std::size_t width) const {
    const auto filled = static_cast<std::size_t>(
        std::clamp(value, 0.0F, 1.0F) * static_cast<float>(width));
    return "\x1b[32m[" + std::string(filled, '#') +
           std::string(width - filled, '-') + "]\x1b[0m";
}

}  // namespace als
