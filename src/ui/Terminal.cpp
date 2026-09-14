#include "als/ui/Terminal.h"

#include <chrono>
#include <iostream>
#include <thread>

#ifdef _WIN32
#include <conio.h>
#else
#include <sys/select.h>
#include <unistd.h>
#endif

namespace als {

Terminal::Terminal(std::istream& input, std::ostream& output)
    : input_(input), output_(output) {
    enableInteractiveMode();
    setCursorVisible(false);
    clear();
}

Terminal::~Terminal() {
    setCursorVisible(true);
    restoreMode();
    output_ << "\x1b[0m\n";
}

void Terminal::enableInteractiveMode() {
#ifdef _WIN32
    inputHandle_ = GetStdHandle(STD_INPUT_HANDLE);
    outputHandle_ = GetStdHandle(STD_OUTPUT_HANDLE);
    if (inputHandle_ == INVALID_HANDLE_VALUE ||
        outputHandle_ == INVALID_HANDLE_VALUE ||
        !GetConsoleMode(inputHandle_, &originalInputMode_) ||
        !GetConsoleMode(outputHandle_, &originalOutputMode_)) {
        return;
    }
    const DWORD inputMode =
        originalInputMode_ & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
    SetConsoleMode(inputHandle_, inputMode);
    SetConsoleMode(
        outputHandle_,
        originalOutputMode_ | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    interactiveMode_ = true;
#else
    if (!isatty(STDIN_FILENO) || tcgetattr(STDIN_FILENO, &originalMode_) != 0) {
        return;
    }
    termios raw = originalMode_;
    raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    interactiveMode_ = true;
#endif
}

void Terminal::restoreMode() {
    if (!interactiveMode_) {
        return;
    }
#ifdef _WIN32
    SetConsoleMode(inputHandle_, originalInputMode_);
    SetConsoleMode(outputHandle_, originalOutputMode_);
#else
    tcsetattr(STDIN_FILENO, TCSANOW, &originalMode_);
#endif
    interactiveMode_ = false;
}

KeyEvent Terminal::readKey(int timeoutMilliseconds) {
    if (!interactiveMode_) {
        const int streamed = input_.get();
        if (streamed == std::char_traits<char>::eof()) {
            return {Key::Escape, '\0'};
        }
        if (streamed == '\r' || streamed == '\n') {
            return {Key::Enter, '\0'};
        }
        return {Key::Character, static_cast<char>(streamed)};
    }

#ifdef _WIN32
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeoutMilliseconds);
    while (!_kbhit()) {
        if (std::chrono::steady_clock::now() >= deadline) {
            return {};
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    int value = _getch();
    if (value == 0 || value == 224) {
        value = _getch();
        switch (value) {
            case 72: return {Key::Up, '\0'};
            case 80: return {Key::Down, '\0'};
            case 75: return {Key::Left, '\0'};
            case 77: return {Key::Right, '\0'};
            default: return {};
        }
    }
#else
    fd_set descriptors;
    FD_ZERO(&descriptors);
    FD_SET(STDIN_FILENO, &descriptors);
    timeval timeout{
        timeoutMilliseconds / 1000,
        (timeoutMilliseconds % 1000) * 1000
    };
    if (select(STDIN_FILENO + 1, &descriptors, nullptr, nullptr, &timeout) <= 0) {
        return {};
    }
    unsigned char value = 0;
    if (::read(STDIN_FILENO, &value, 1) != 1) {
        return {};
    }
    if (value == 27) {
        unsigned char sequence[2]{};
        if (::read(STDIN_FILENO, sequence, 2) == 2 && sequence[0] == '[') {
            switch (sequence[1]) {
                case 'A': return {Key::Up, '\0'};
                case 'B': return {Key::Down, '\0'};
                case 'C': return {Key::Right, '\0'};
                case 'D': return {Key::Left, '\0'};
            }
        }
        return {Key::Escape, '\0'};
    }
#endif

    if (value == 13 || value == 10) {
        return {Key::Enter, '\0'};
    }
    if (value == 27) {
        return {Key::Escape, '\0'};
    }
    return {Key::Character, static_cast<char>(value)};
}

std::string Terminal::readLine(
    const std::string& prompt, const std::string& current) {
    setCursorVisible(true);
    clear();
    restoreMode();
    output_ << prompt << "\nActual: " << current << "\nNuevo valor: " << std::flush;
    std::string value;
    std::getline(input_, value);
    enableInteractiveMode();
    setCursorVisible(false);
    return value.empty() ? current : value;
}

void Terminal::clear() {
    output_ << "\x1b[2J\x1b[H" << std::flush;
}

void Terminal::setCursorVisible(bool visible) {
    output_ << (visible ? "\x1b[?25h" : "\x1b[?25l") << std::flush;
}

}  // namespace als
