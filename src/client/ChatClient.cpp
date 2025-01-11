#include <iostream>
#include <utility>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "ChatClient.h"
#include "../common/Commands.h"
#include "../common/Constants.h"
#include "../common/WinsockInitializer.h"

using std::cout;
using std::cin;
using std::string;
using std::runtime_error;

ChatClient::ChatClient(const string &ip, const unsigned short &port, const string &username): ip(ip), port(port),
    username(username), clientSocket(0) {
}

ChatClient::~ChatClient() {
    shutdown();
}

void ChatClient::run() {
    WinsockInitializer winsock;

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        WSACleanup();
        throw runtime_error("Socket creation failed. Error: " + WSAGetLastError());
    }

    setupConnection();
}

void ChatClient::shutdown() const {
    if (clientSocket != INVALID_SOCKET) {
        closesocket(clientSocket);
        WSACleanup();
    }
}

void ChatClient::setupConnection() {
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &serverAddress.sin_addr) != 1) {
        shutdown();
        throw runtime_error("Invalid IP address format.");
    }

    if (connect(clientSocket, reinterpret_cast<sockaddr *>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        shutdown();
        throw runtime_error("Connection failed. Error: " + WSAGetLastError());
    }

    if (send(clientSocket, username.c_str(), username.size(), 0) == SOCKET_ERROR) {
        shutdown();
        throw runtime_error("Couldn't send the username. Error: " + WSAGetLastError());
    }

    cout << "Connected. Type '/help' to see the commands, '/exit' to disconnect.\n\n\n";

    std::thread sendThread(&ChatClient::handleSending, this);
    std::thread receiveThread(&ChatClient::handleReceiving, this);

    sendThread.join();
    receiveThread.join();

    shutdown();
}

void ChatClient::handleSending() const {
    Commands commands("/");

    commands.addCommand("help", "Display available commands", [](const std::vector<string> &) {
        cout << '\n';
        cout << "/help    - Display available commands\n";
        cout << "/exit    - Disconnect from server\n";
        cout << "/whoami  - Display your current session information\n";
    });

    commands.addCommand("exit", "Disconnect from server", [this](const std::vector<string> &) {
        shutdown();
    });

    commands.addCommand("whoami", "Display your current session information", [this](const std::vector<string> &) {
        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
            cout << "Error retrieving hostname. Error: " << WSAGetLastError() << '\n';
            return;
        }

        addrinfo hints{}, *info;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        if (getaddrinfo(hostname, nullptr, &hints, &info) != 0) {
            cout << "Error resolving hostname to IP address. Error: " << WSAGetLastError() << '\n';
            return;
        }

        char ipStr[INET_ADDRSTRLEN];
        const auto *ipv4 = reinterpret_cast<sockaddr_in *>(info->ai_addr);
        inet_ntop(AF_INET, &ipv4->sin_addr, ipStr, INET_ADDRSTRLEN);

        freeaddrinfo(info);

        cout << "Username: " << username << '\n';
        cout << "IP Address: " << ipStr << '\n';
        cout << "Connected server: " << ip << ':' << port << '\n';
    });

    string message;
    while (getline(cin, message)) {
        if (message.rfind('/', 0) == 0) {
            commands.executeCommand(message);
            cout << '\n';
        } else {
            if (message.empty()) continue;
            if (send(clientSocket, message.c_str(), static_cast<int>(message.size()), 0) == SOCKET_ERROR) {
                cout << "Send failed. Error: " << WSAGetLastError() << '\n';
                break;
            }

            removeLastLine();
            printMessage(username + ": " + message);
        }
    }
}

void ChatClient::handleReceiving() {
    while (true) {
        constexpr int BUFFER_SIZE = 1024;
        char buffer[BUFFER_SIZE + 1];
        const int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);

        if (bytesReceived <= 0) {
            cout << "Disconnected from server.\n";
            shutdown();
            break;
        }

        buffer[bytesReceived] = '\0';

        if (const string message(buffer); message == BANNED) {
            cout << "You're banned from this server.\n";
            shutdown();
            break;
        } else if (message == KICKED) {
            cout << "You're kicked from this server.\n";
            shutdown();
            break;
        } {
            std::lock_guard lock(outputMutex);
            printMessage(buffer);
        }
    }
}

void ChatClient::removeLastLine() {
    cout << "\033[A\033[K";
}

void ChatClient::printMessage(const string &message) {
    removeLastLine();
    cout << separator << '\n';

    int start = 0;
    while (start < message.size()) {
        int end = start + COLUMN_WIDTH;
        if (end >= message.size()) end = message.size();
        cout << message.substr(start, end - start) << "\n\n";
        start = end;
    }
}
