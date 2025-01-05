#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>

#define COLUMN_WIDTH 100

std::mutex outputMutex;
std::string input;
std::string username;
std::string ip = "127.0.0.1";
unsigned short port = 8080;
const std::string separator(COLUMN_WIDTH, '-');

void removeLastLine();

void writeMessage(const std::string &message);

int main() {
    std::cout << "Enter username: ";
    std::getline(std::cin, input);
    if (!input.empty()) {
        username = input;
    }

    std::cout << "Enter server IP (default " << ip << "): ";
    std::getline(std::cin, input);
    if (!input.empty()) {
        ip = input;
    }

    std::cout << "Enter port (default " << port << "): ";
    std::getline(std::cin, input);
    if (!input.empty()) {
        try {
            if (const unsigned short newPort = std::stoi(input); newPort > 0 && newPort <= 65535) port = newPort;
            else std::cout << "Invalid port range. Using default.\n";
        } catch (...) {
            std::cout << "Invalid input. Using default port.\n";
        }
    }

    WSADATA wsaData;
    if (const int result = WSAStartup(MAKEWORD(2, 2), &wsaData); result != 0) {
        std::cout << "WSAStartup failed. Error: " << result << '\n';
        return 1;
    }

    const auto clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cout << "Socket creation failed. Error: " << WSAGetLastError() << '\n';
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &serverAddress.sin_addr) != 1) {
        std::cout << "Invalid IP address format.\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    if (connect(clientSocket, reinterpret_cast<sockaddr *>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cout << "Connection failed. Error: " << WSAGetLastError() << '\n';
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    if (username.empty() || username.size() > 100) {
        std::cout << "Invalid username. Please restart and use a valid username (1-100 characters).\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    if (send(clientSocket, username.c_str(), username.size(), 0) == SOCKET_ERROR) {
        std::cout << "Couldn't send the username. Error: " << WSAGetLastError() << '\n';
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected. Type '/help' to see the commands, '/exit' to disconnect.\n\n\n";

    std::thread sendThread([&] {
        std::string message;
        while (std::getline(std::cin, message)) {
            if (message == "/exit") {
                shutdown(clientSocket, SD_SEND);
                break;
            }

            if (message == "/help") {
                std::cout << "\nThere'll be something here.\n\n\n";
                continue;
            }

            if (send(clientSocket, message.c_str(), static_cast<int>(message.size()), 0) == SOCKET_ERROR) {
                std::lock_guard lock(outputMutex);
                std::cout << "Send failed. Error: " << WSAGetLastError() << '\n';
                break;
            }

            removeLastLine();
            writeMessage(username + ": " + message);
        }
    });

    std::thread receiveThread([&] {
        while (true) {
            char buffer[1024 + 1];
            const int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

            if (bytesReceived <= 0) {
                std::cout << "Disconnected from server.\n";
                break;
            }

            buffer[bytesReceived] = '\0'; {
                std::lock_guard lock(outputMutex);
                writeMessage(buffer);
            }
        }
    });

    sendThread.join();
    receiveThread.join();

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}

void removeLastLine() {
    std::cout << "\033[A\033[K";
}

void writeMessage(const std::string &message) {
    removeLastLine();
    std::cout << separator << '\n';

    int start = 0;
    while (start < message.size()) {
        int end = start + COLUMN_WIDTH;
        if (end >= message.size()) end = message.size();
        std::cout << message.substr(start, end - start) << "\n\n";
        start = end;
    }
}
