#include "als/application/Application.h"
#include "als/ui/Dashboard.h"
#include "als/ui/Terminal.h"

#include <iostream>

int main() {
    als::Application application;
    if (!application.start()) {
        std::cerr << application.lastError() << '\n';
        return 1;
    }

    als::Terminal terminal(std::cin, std::cout);
    als::Dashboard dashboard(application, terminal, std::cout);
    dashboard.run();
    return 0;
}
