#include <iostream>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <mutex>
#include <atomic>

#define PORT 8080

std::vector<SOCKET> clients;
std::mutex clientsMutex;
std::atomic isRunning(true);

void handleClient(SOCKET clientSocket);

void broadcastMessage(const std::string &message, SOCKET senderSocket);

void listenExit(SOCKET serverSocket);

int main() {
    WSADATA wsaData;
    if (const int result = WSAStartup(MAKEWORD(2, 2), &wsaData); result != 0) {
        std::cout << "WSAStartup failed. Error: " << result << '\n';
        return 1;
    }

    const auto serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cout << "Socket creation failed. Error: " << WSAGetLastError() << '\n';
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(PORT);

    if (bind(serverSocket, reinterpret_cast<sockaddr *>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cout << "Bind failed. Error: " << WSAGetLastError() << '\n';
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        std::cout << "Listen failed. Error: " << WSAGetLastError() << '\n';
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server listening on port " << PORT << "\nPress Enter to stop the server...\n";

    sockaddr_in clientAddress{};
    socklen_t clientAddressLength = sizeof(clientAddress);

    std::thread(listenExit, serverSocket).detach();

    while (isRunning) {
        const SOCKET clientSocket = accept(serverSocket, reinterpret_cast<sockaddr *>(&clientAddress),
                                           &clientAddressLength);
        if (clientSocket == INVALID_SOCKET) {
            if (!isRunning) break;
            std::cout << "Accept failed. Error: " << WSAGetLastError() << '\n';
            continue;
        } {
            std::lock_guard lock(clientsMutex);
            clients.push_back(clientSocket);
        }

        std::thread(handleClient, clientSocket).detach();
    }

    std::cout << "Server shutting down...\n";
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}

void handleClient(const SOCKET clientSocket) {
    std::cout << "Client connected: " << clientSocket << '\n';

    char buffer[1024];
    while (true) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            std::cout << "Client disconnected.\n";
            break;
        }

        buffer[bytesReceived] = '\0';
        std::cout << "Received: " << buffer << '\n';

        broadcastMessage(buffer, clientSocket);
    } {
        std::lock_guard lock(clientsMutex);
        std::erase(clients, clientSocket);
    }

    closesocket(clientSocket);
}

void broadcastMessage(const std::string &message, SOCKET senderSocket) {
    std::lock_guard lock(clientsMutex);
    for (const SOCKET client: clients) {
        if (client != senderSocket) {
            if (send(client, message.c_str(), message.size(), 0) == SOCKET_ERROR) {
                std::cout << "Send failed to a client. Error: " << WSAGetLastError() << '\n';
            }
        }
    }
}

void listenExit(const SOCKET serverSocket) {
    std::cin.get();
    isRunning = false; {
        std::lock_guard lock(clientsMutex);
        for (const SOCKET client: clients) {
            closesocket(client);
        }
    }

    closesocket(serverSocket);
    WSACleanup();
}
