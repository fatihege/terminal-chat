#pragma once

#include <string>
#include <winsock2.h>

class ClientHandler {
public:
    ClientHandler(const ClientHandler &) = delete;

    ClientHandler &operator=(const ClientHandler &) = delete;

    ClientHandler(ClientHandler &&other) noexcept;

    ClientHandler &operator=(ClientHandler &&other) noexcept;

    ClientHandler(SOCKET, const std::string &, const std::string &);

    ~ClientHandler();

    [[nodiscard]] SOCKET getSocket() const;

    [[nodiscard]] std::string getInfo() const;

    [[nodiscard]] std::string getIp() const;

    void sendBanMessage() const;

    void sendKickMessage() const;

private:
    SOCKET socket;
    std::string ip;
    std::string username;
};
