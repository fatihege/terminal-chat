#include <iostream>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <mutex>
#include <atomic>
#include <fstream>
#include <string>

#define BANS_FILE "bans"

using std::string;
using std::cout;
using std::cin;
using std::getline;
using std::vector;
using std::thread;

struct Client {
    SOCKET socket;
    string username;
    string ip;
};

string input;
vector<Client> clients;
vector<string> bans;
std::mutex clientsMutex;
std::atomic isRunning(true);
unsigned short port = 8080;
const string banMessage = "banned";
const string kickMessage = "kicked";

void initialConfiguration();

void acceptClients(const auto &serverSocket);

int bindAndListen(const auto &serverSocket);

void loadBans();

void banClient(const SOCKET &socket);

string clientInfo(const Client &client);

string getClientIp(const SOCKET &clientSocket);

void removeClient(SOCKET clientSocket);

void handleClient(SOCKET clientSocket);

void broadcastMessage(const string &message, SOCKET senderSocket);

void listenCommands(SOCKET serverSocket);

void setCommands();

int main() {
    initialConfiguration();
    loadBans();

    WSADATA wsaData;
    if (const int result = WSAStartup(MAKEWORD(2, 2), &wsaData); result != 0) {
        cout << "WSAStartup failed. Error: " << result << '\n';
        cin.get();
        return 1;
    }

    const auto serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        cout << "Socket creation failed. Error: " << WSAGetLastError() << '\n';
        WSACleanup();
        cin.get();
        return 1;
    }

    if (const int error = bindAndListen(serverSocket); error != 0) return error;
    cout << "Server listening on port " << port << "\nType 'help' to see the commands, 'exit' to stop the server.\n\n";
    acceptClients(serverSocket);

    cout << "Server shutting down...\n";
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}

void initialConfiguration() {
    cout << "Choose port (default " << port << "): ";
    getline(cin, input);
    if (!input.empty()) {
        try {
            if (const unsigned short newPort = std::stoi(input); newPort > 0 && newPort <= 65535) port = newPort;
            else cout << "Invalid port range. Using default port.\n";
        } catch (...) {
            cout << "Invalid input. Using default port.\n";
        }
    }
}

void acceptClients(const auto &serverSocket) {
    sockaddr_in clientAddress{};
    socklen_t clientAddressLength = sizeof(clientAddress);

    thread(listenCommands, serverSocket).detach();

    while (isRunning) {
        const SOCKET clientSocket = accept(serverSocket, reinterpret_cast<sockaddr *>(&clientAddress),
                                           &clientAddressLength);
        if (clientSocket == INVALID_SOCKET) {
            if (!isRunning) break;
            cout << "Accept failed. Error: " << WSAGetLastError() << '\n';
            continue;
        }

        thread(handleClient, clientSocket).detach();
    }
}

int bindAndListen(const auto &serverSocket) {
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(port);

    if (bind(serverSocket, reinterpret_cast<sockaddr *>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        cout << "Bind failed. Error: " << WSAGetLastError() << '\n';
        closesocket(serverSocket);
        WSACleanup();
        cin.get();
        return 1;
    }

    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        cout << "Listen failed. Error: " << WSAGetLastError() << '\n';
        closesocket(serverSocket);
        WSACleanup();
        cin.get();
        return 1;
    }

    return 0;
}


void loadBans() {
    bans.clear();
    std::ifstream bansInput(BANS_FILE);
    if (!bansInput) {
        std::ofstream bansOutput(BANS_FILE);
        bansOutput.close();
        cout << "Bans file not found. Created a new file.\n";
    } else {
        string line;
        cout << "Loading banned IPs...\n";
        while (getline(bansInput, line)) bans.push_back(line);
        cout << bans.size() << " bans loaded.\n";
    }
    bansInput.close();
}

string clientInfo(const Client &client) {
    return client.ip + " " + client.username + " (" + std::to_string(client.socket) + ")";
}

void removeClient(const SOCKET clientSocket) {
    std::erase_if(clients, [clientSocket](const Client &client) { return client.socket == clientSocket; });
}

string getClientIp(const SOCKET &clientSocket) {
    sockaddr_in clientAddress;
    int clientAddressSize = sizeof(clientAddress);
    if (getpeername(clientSocket, reinterpret_cast<sockaddr *>(&clientAddress), &clientAddressSize) == 0) {
        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddress.sin_addr, clientIp, INET_ADDRSTRLEN);
        return clientIp;
    }

    return "";
}

void handleClient(const SOCKET clientSocket) {
    const string ip = getClientIp(clientSocket);
    if (ip == "") {
        cout << "Failed to retrieve client address. Error: " << WSAGetLastError() << '\n';
        return;
    }

    char buffer[1024];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived <= 0) {
        cout << "Failed to receive username from client.\n";
        closesocket(clientSocket);
        return;
    }

    buffer[bytesReceived] = '\0';
    const string username(buffer);
    const Client client = {clientSocket, username, ip};

    if (std::ranges::find(bans, client.ip) != bans.end()) {
        banClient(client.socket);
        cout << "Banned IP tried to connect: " << clientInfo(client) << '\n';
        return;
    } {
        std::lock_guard lock(clientsMutex);
        removeClient(client.socket);
        clients.push_back(client);
    }

    cout << "Client connected: " << clientInfo(client) << '\n';

    while (true) {
        bytesReceived = recv(client.socket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            std::lock_guard lock(clientsMutex);
            cout << "Client disconnected: " << clientInfo(client) << "\n";
            break;
        }

        buffer[bytesReceived] = '\0';
        broadcastMessage(username + ": " + buffer, client.socket);
    } {
        std::lock_guard lock(clientsMutex);
        removeClient(client.socket);
    }

    closesocket(client.socket);
}

void broadcastMessage(const string &message, const SOCKET senderSocket) {
    std::lock_guard lock(clientsMutex);
    for (const Client &client: clients) {
        if (client.socket != senderSocket) {
            if (send(client.socket, message.c_str(), static_cast<int>(message.size()), 0) == SOCKET_ERROR) {
                cout << "Send failed to client " << clientInfo(client) << ". Error: " << WSAGetLastError() << '\n';
            }
        }
    }
}

void banClient(const SOCKET &socket) {
    send(socket, banMessage.c_str(), static_cast<int>(banMessage.size()), 0);
    closesocket(socket);
}

void kickClient(const SOCKET &socket) {
    send(socket, kickMessage.c_str(), static_cast<int>(kickMessage.size()), 0);
    closesocket(socket);
}

void listenCommands(const SOCKET serverSocket) {
    string input;
    while (true) {
        getline(cin, input);
        if (input == "exit") {
            isRunning = false;
            break;
        }

        if (input == "list") {
        }

        if (input == "bans") {
            std::lock_guard lock(clientsMutex);
            for (const string &ip: bans) cout << ip << '\n';
            cout << "- end (" << bans.size() << ") -\n\n";
            continue;
        }

        if (input == "refresh bans") {
            std::lock_guard lock(clientsMutex);
            loadBans();
            for (const Client &client: clients) {
                if (std::ranges::find(bans, client.ip) != bans.end()) banClient(client.socket);
            }
            cout << '\n';
            continue;
        }

        if (input == "clear bans") {
            std::lock_guard lock(clientsMutex);
            bans.clear();

            cout << "Removing all bans...\n";
            std::ofstream bansOutput(BANS_FILE, std::ios::trunc);
            bansOutput.close();
            cout << "Bans removed.\n\n";
        }

        if (input.starts_with("ban ") && input.size() > 4) {
            std::lock_guard lock(clientsMutex);
            string ipToBan = input.substr(4);
            bans.push_back(ipToBan);

            std::ofstream bansOutput(BANS_FILE, std::ios::app);
            bansOutput << ipToBan << '\n';
            bansOutput.close();

            for (const Client &client: clients) {
                if (ipToBan == client.ip) banClient(client.socket);
            }

            cout << "IP " << ipToBan << " has been banned.\n\n";
            continue;
        }

        if (input.starts_with("unban ") && input.size() > 6) {
            std::lock_guard lock(clientsMutex);
            string ipToUnban = input.substr(6);

            auto it = std::remove(bans.begin(), bans.end(), ipToUnban);
            if (it == bans.end()) {
                cout << "IP " << ipToUnban << " is not banned.\n\n";
                continue;
            }

            bans.erase(it, bans.end());

            std::ofstream bansOutput(BANS_FILE, std::ios::trunc);
            for (const string &ip: bans) bansOutput << ip << '\n';
            bansOutput.close();

            cout << "IP " << ipToUnban << " has been unbanned.\n\n";
            continue;
        }

        if (input.starts_with("kick ") && input.size() > 5) {
            std::lock_guard lock(clientsMutex);
            string ipToKick = input.substr(5);
            bool kicked = false;

            for (const Client &client: clients) {
                if (ipToKick == client.ip) {
                    kicked = true;
                    kickClient(client.socket);
                }
            }

            if (kicked) cout << "IP " << ipToKick << " has been kicked.\n\n";
            else cout << "No client connected by the IP " << ipToKick << ".\n\n";

            continue;
        }

        if (input == "help") {
            std::lock_guard lock(clientsMutex);
            cout << "There'll be commands here.\n\n";
            continue;
        }
    }

    std::lock_guard lock(clientsMutex);
    for (const Client &client: clients) {
        closesocket(client.socket);
    }

    closesocket(serverSocket);
    WSACleanup();
}
