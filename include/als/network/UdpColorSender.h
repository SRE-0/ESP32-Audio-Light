#pragma once

#include "als/config/Config.h"
#include "als/network/IColorSender.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#endif

#include <mutex>

namespace als {

class UdpColorSender final : public IColorSender {
public:
    UdpColorSender() = default;
    ~UdpColorSender() override;

    UdpColorSender(const UdpColorSender&) = delete;
    UdpColorSender& operator=(const UdpColorSender&) = delete;

    bool initialize(const Config& config);
    bool send(const Pixel& pixel) override;
    void close();

private:
    void closeUnlocked();

    std::mutex mutex_;
#ifdef _WIN32
    SOCKET socket_ = INVALID_SOCKET;
    bool winsockInitialized_ = false;
#else
    int socket_ = -1;
#endif
    sockaddr_in destination_{};
};

}  // namespace als
