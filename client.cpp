#include <iostream>
#include <string>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>

#define PORT 8080

int main() {
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
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);
    serverAddress.sin_port = htons(PORT);

    if (connect(clientSocket, reinterpret_cast<sockaddr *>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cout << "Connection failed. Error: " << WSAGetLastError() << '\n';
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::thread sendThread([&] {
        std::string message;
        while (std::getline(std::cin, message)) {
            if (message == "exit") {
                shutdown(clientSocket, SD_SEND);
                break;
            }
            if (send(clientSocket, message.c_str(), message.size(), 0) == SOCKET_ERROR) {
                std::cout << "Send failed. Error: " << WSAGetLastError() << '\n';
                break;
            }
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
            buffer[bytesReceived] = '\0';
            std::cout << buffer << std::endl;
        }
    });

    sendThread.join();
    receiveThread.join();

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}
