#pragma once

#include "als/application/Application.h"
#include "als/ui/Terminal.h"

#include <cstddef>
#include <iosfwd>
#include <string>

namespace als {

class Dashboard {
public:
    Dashboard(
        Application& application,
        Terminal& terminal,
        std::ostream& output);

    void run();

private:
    void render();
    void handle(const KeyEvent& event);
    void adjustSelected(int direction);
    void activateSelected();
    void editIp();
    void editPort();
    bool apply(const Config& config);
    std::string selectedRow(std::size_t row, const std::string& text) const;
    std::string levelBar(float value, std::size_t width) const;

    Application& application_;
    Terminal& terminal_;
    std::ostream& output_;
    std::size_t selected_ = 0;
    bool exitRequested_ = false;
    std::string notice_;
};

}  // namespace als
