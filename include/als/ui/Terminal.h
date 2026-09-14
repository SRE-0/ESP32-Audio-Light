#pragma once

#include <iosfwd>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <termios.h>
#endif

namespace als {

enum class Key {
    None,
    Up,
    Down,
    Left,
    Right,
    Enter,
    Escape,
    Character
};

struct KeyEvent {
    Key key = Key::None;
    char character = '\0';
};

class Terminal {
public:
    Terminal(std::istream& input, std::ostream& output);
    ~Terminal();

    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;

    KeyEvent readKey(int timeoutMilliseconds);
    std::string readLine(const std::string& prompt, const std::string& current);
    void clear();
    void setCursorVisible(bool visible);

private:
    void enableInteractiveMode();
    void restoreMode();

    std::istream& input_;
    std::ostream& output_;
    bool interactiveMode_ = false;
#ifdef _WIN32
    HANDLE inputHandle_ = INVALID_HANDLE_VALUE;
    HANDLE outputHandle_ = INVALID_HANDLE_VALUE;
    DWORD originalInputMode_ = 0;
    DWORD originalOutputMode_ = 0;
#else
    termios originalMode_{};
#endif
};

}  // namespace als
