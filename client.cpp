#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>

#define COLUMN_WIDTH 100

using std::string;
using std::cout;
using std::cin;
using std::getline;
using std::thread;

std::mutex outputMutex;
string input;
string username;
string ip = "127.0.0.1";
unsigned short port = 8080;
const string separator(COLUMN_WIDTH, '-');
const string banMessage = "banned";
const string kickMessage = "kicked";

void removeLastLine();

void writeMessage(const string &message);

int main() {
    cout << "Enter username: ";
    getline(cin, input);
    if (!input.empty()) {
        username = input;
    }

    for (const char c: username) {
        if (!isalnum(c)) {
            cout << "Invalid username. Use alphanumeric characters only.\n";
            cin.get();
            return 1;
        }
    }

    if (username.empty() || username.size() > 100) {
        cout << "Invalid username. Please restart and use a valid username (1-100 characters).\n";
        cin.get();
        return 1;
    }

    cout << "Enter server IP (default " << ip << "): ";
    getline(cin, input);
    if (!input.empty()) {
        ip = input;
    }

    cout << "Enter port (default " << port << "): ";
    getline(cin, input);
    if (!input.empty()) {
        try {
            if (const unsigned short newPort = std::stoi(input); newPort > 0 && newPort <= 65535) port = newPort;
            else cout << "Invalid port range. Using default.\n";
        } catch (...) {
            cout << "Invalid input. Using default port.\n";
        }
    }

    WSADATA wsaData;
    if (const int result = WSAStartup(MAKEWORD(2, 2), &wsaData); result != 0) {
        cout << "WSAStartup failed. Error: " << result << '\n';
        cin.get();
        return 1;
    }

    const auto clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        cout << "Socket creation failed. Error: " << WSAGetLastError() << '\n';
        WSACleanup();
        cin.get();
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &serverAddress.sin_addr) != 1) {
        cout << "Invalid IP address format.\n";
        closesocket(clientSocket);
        WSACleanup();
        cin.get();
        return 1;
    }

    if (connect(clientSocket, reinterpret_cast<sockaddr *>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        cout << "Connection failed. Error: " << WSAGetLastError() << '\n';
        closesocket(clientSocket);
        WSACleanup();
        cin.get();
        return 1;
    }

    if (send(clientSocket, username.c_str(), username.size(), 0) == SOCKET_ERROR) {
        cout << "Couldn't send the username. Error: " << WSAGetLastError() << '\n';
        closesocket(clientSocket);
        WSACleanup();
        cin.get();
        return 1;
    }

    cout << "Connected. Type '/help' to see the commands, '/exit' to disconnect.\n\n\n";

    thread sendThread([&] {
        string message;
        while (getline(cin, message)) {
            if (message == "/exit") {
                shutdown(clientSocket, SD_SEND);
                break;
            }

            if (message == "/help") {
                cout << "\nThere'll be something here.\n\n\n";
                continue;
            }

            if (send(clientSocket, message.c_str(), static_cast<int>(message.size()), 0) == SOCKET_ERROR) {
                std::lock_guard lock(outputMutex);
                cout << "Send failed. Error: " << WSAGetLastError() << '\n';
                break;
            }

            removeLastLine();
            writeMessage(username + ": " + message);
        }
    });

    thread receiveThread([&] {
        while (true) {
            constexpr int BUFFER_SIZE = 1024;
            char buffer[BUFFER_SIZE + 1];
            const int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);

            if (bytesReceived <= 0) {
                cout << "Disconnected from server.\n";
                closesocket(clientSocket);
                WSACleanup();
                break;
            }

            buffer[bytesReceived] = '\0';

            if (const string message(buffer); message == banMessage) {
                cout << "You're banned from this server.\n";
                closesocket(clientSocket);
                WSACleanup();
                break;
            } else if (message == kickMessage) {
                cout << "You're kicked from this server.\n";
                closesocket(clientSocket);
                WSACleanup();
                break;
            } {
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
    cout << "\033[A\033[K";
}

void writeMessage(const string &message) {
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
