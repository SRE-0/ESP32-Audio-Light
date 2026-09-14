#include "als/network/UdpColorSender.h"

#include <cstring>
#include <iostream>

#ifndef _WIN32
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace als {

UdpColorSender::~UdpColorSender() {
    close();
}

bool UdpColorSender::initialize(const Config& config) {
    sockaddr_in newDestination{};
    newDestination.sin_family = AF_INET;
    newDestination.sin_port = htons(static_cast<std::uint16_t>(config.port));
#ifdef _WIN32
    const int addressResult =
        InetPtonA(AF_INET, config.ip.c_str(), &newDestination.sin_addr);
#else
    const int addressResult =
        inet_pton(AF_INET, config.ip.c_str(), &newDestination.sin_addr);
#endif
    if (addressResult != 1) {
        return false;
    }

    const std::lock_guard<std::mutex> lock(mutex_);
    closeUnlocked();
#ifdef _WIN32
    WSADATA winsockData;
    const int startupResult = WSAStartup(MAKEWORD(2, 2), &winsockData);
    if (startupResult != 0) {
        std::cerr << "[NET] WSAStartup failed: " << startupResult << '\n';
        return false;
    }
    winsockInitialized_ = true;
    socket_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_ == INVALID_SOCKET) {
        std::cerr << "[NET] Failed to create UDP socket: "
                  << WSAGetLastError() << '\n';
        closeUnlocked();
        return false;
    }
#else
    socket_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_ < 0) {
        std::cerr << "[NET] Failed to create UDP socket\n";
        return false;
    }
#endif

    destination_ = newDestination;
    return true;
}

bool UdpColorSender::send(const Pixel& pixel) {
    const std::lock_guard<std::mutex> lock(mutex_);
#ifdef _WIN32
    const int bytesSent = sendto(
        socket_, reinterpret_cast<const char*>(&pixel), sizeof(pixel), 0,
        reinterpret_cast<const sockaddr*>(&destination_), sizeof(destination_));
    if (bytesSent == SOCKET_ERROR) {
        std::cerr << "[NET] sendto failed: " << WSAGetLastError() << '\n';
        return false;
    }
#else
    const auto bytesSent = sendto(
        socket_, &pixel, sizeof(pixel), 0,
        reinterpret_cast<const sockaddr*>(&destination_), sizeof(destination_));
    if (bytesSent < 0) {
        std::cerr << "[NET] sendto failed\n";
        return false;
    }
#endif
    return static_cast<long long>(bytesSent) ==
           static_cast<long long>(sizeof(pixel));
}

void UdpColorSender::close() {
    const std::lock_guard<std::mutex> lock(mutex_);
    closeUnlocked();
}

void UdpColorSender::closeUnlocked() {
#ifdef _WIN32
    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }
    if (winsockInitialized_) {
        WSACleanup();
        winsockInitialized_ = false;
    }
#else
    if (socket_ >= 0) {
        ::close(socket_);
        socket_ = -1;
    }
#endif
}

}  // namespace als
