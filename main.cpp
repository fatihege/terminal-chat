#include <iostream>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <mutex>
#include <atomic>
#include <string>

struct Client {
    SOCKET socket;
    std::string username;
};

std::string input;
std::vector<Client> clients;
std::mutex clientsMutex;
std::atomic isRunning(true);
unsigned short port = 8080;

void removeClient(SOCKET clientSocket);

void handleClient(SOCKET clientSocket);

void broadcastMessage(const std::string &message, SOCKET senderSocket);

void listenExit(SOCKET serverSocket);

int main() {
    std::cout << "Choose port (default " << port << "): ";
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

    const auto serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cout << "Socket creation failed. Error: " << WSAGetLastError() << '\n';
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(port);

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

    std::cout << "Server listening on port " << port << "\nType 'exit' and press Enter to stop the server...\n";

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
            clients.push_back(Client{clientSocket,});
        }

        std::thread(handleClient, clientSocket).detach();
    }

    std::cout << "Server shutting down...\n";
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}

void removeClient(const SOCKET clientSocket) {
    std::erase_if(clients, [clientSocket](const Client &client) { return client.socket == clientSocket; });
}

void handleClient(const SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived <= 0) {
        std::cout << "Failed to receive username from client.\n";
        closesocket(clientSocket);
        return;
    }
    buffer[bytesReceived] = '\0';
    const std::string username(buffer); {
        std::lock_guard lock(clientsMutex);
        removeClient(clientSocket);
        clients.push_back({clientSocket, username});
    }

    std::cout << "Client connected: " << username << " (" << clientSocket << ")\n";

    while (true) {
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            std::cout << "Client disconnected: " << username << " (" << clientSocket << ")\n";
            break;
        }

        buffer[bytesReceived] = '\0';
        std::string message = username + ": " + buffer;

        std::cout << "Received: " << message << '\n';
        broadcastMessage(message, clientSocket);
    } {
        std::lock_guard lock(clientsMutex);
        removeClient(clientSocket);
    }

    closesocket(clientSocket);
}

void broadcastMessage(const std::string &message, const SOCKET senderSocket) {
    std::lock_guard lock(clientsMutex);
    std::cout << "Socket count: " << clients.size() << '\n';
    for (const auto &[socket, username]: clients) {
        if (socket != senderSocket) {
            if (send(socket, message.c_str(), static_cast<int>(message.size()), 0) == SOCKET_ERROR) {
                std::cout << "Send failed to client " << username << ". Error: " << WSAGetLastError() << '\n';
            }
        }
    }
}

void listenExit(const SOCKET serverSocket) {
    std::string input;
    while (true) {
        std::getline(std::cin, input);
        if (input == "exit") {
            isRunning = false;
            break;
        }
    }

    std::lock_guard lock(clientsMutex);
    for (const auto &[socket, _]: clients) {
        closesocket(socket);
    }

    closesocket(serverSocket);
    WSACleanup();
}
