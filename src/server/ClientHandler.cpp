#include <utility>

#include <iostream>

#include "ClientHandler.h"
#include "../common/Constants.h"

ClientHandler::ClientHandler(ClientHandler &&other) noexcept: socket(other.socket), ip(std::move(other.ip)),
                                                              username(std::move(other.username)) {
    other.socket = INVALID_SOCKET;
}

ClientHandler &ClientHandler::operator=(ClientHandler &&other) noexcept {
    if (this != &other) {
        if (socket != INVALID_SOCKET) closesocket(socket);
        socket = other.socket;
        ip = std::move(other.ip);
        username = std::move(other.username);
        other.socket = INVALID_SOCKET;
    }
    return *this;
}

ClientHandler::ClientHandler(const SOCKET socket, const std::string &username, const std::string &ip): socket(socket),
    ip(ip), username(username) {
}

ClientHandler::~ClientHandler() {
    if (socket != INVALID_SOCKET) closesocket(socket);
}

SOCKET ClientHandler::getSocket() const {
    return socket;
}

std::string ClientHandler::getInfo() const {
    return ip + " " + username + " (" + std::to_string(socket) + ")";
}

std::string ClientHandler::getIp() const {
    return ip;
}

void ClientHandler::sendBanMessage() const {
    send(socket, BANNED, static_cast<int>(std::strlen(BANNED)), 0);
    closesocket(socket);
}

void ClientHandler::sendKickMessage() const {
    send(socket, KICKED, static_cast<int>(std::strlen(KICKED)), 0);
    closesocket(socket);
}
