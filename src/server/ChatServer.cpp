#include <fstream>
#include <iostream>
#include <ws2tcpip.h>

#include "ChatServer.h"
#include "../common/Commands.h"
#include "../common/Constants.h"
#include "../common/WinsockInitializer.h"

using std::cout;
using std::string;
using std::runtime_error;

ChatServer::ChatServer(const unsigned short &port): port(port), serverSocket(0), isRunning(false) {
}

ChatServer::~ChatServer() {
    cout << "Server shutting down...\n";
    shutdown();
}

void ChatServer::run() {
    loadBans();

    WinsockInitializer winsock;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        WSACleanup();
        throw runtime_error("Socket creation failed. Error: " + WSAGetLastError());
    }

    initializeSocket();
    isRunning = true;
    cout << "Server listening on port " << port << "\nType 'help' to see the commands, 'exit' to stop the server.\n\n";
    acceptClientConnections();
}

void ChatServer::shutdown() {
    isRunning = false;
    closesocket(serverSocket);
    WSACleanup();
}

void ChatServer::initializeSocket() {
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(port);

    if (bind(serverSocket, reinterpret_cast<sockaddr *>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        shutdown();
        throw runtime_error("Bind failed. Error: " + WSAGetLastError());
    }

    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        shutdown();
        throw runtime_error("Listen failed. Error: " + WSAGetLastError());
    }
}

void ChatServer::acceptClientConnections() {
    sockaddr_in clientAddress{};
    socklen_t clientAddressLength = sizeof(clientAddress);

    std::thread(&ChatServer::processServerCommands, this).detach();

    while (isRunning) {
        const SOCKET clientSocket = accept(serverSocket, reinterpret_cast<sockaddr *>(&clientAddress),
                                           &clientAddressLength);
        if (clientSocket == INVALID_SOCKET) {
            if (!isRunning) break;
            cout << "Accept failed. Error: " << WSAGetLastError() << '\n';
            continue;
        }

        std::thread(&ChatServer::processClientConnection, this, clientSocket).detach();
    }
}

void ChatServer::processClientConnection(const SOCKET clientSocket) {
    const string ip = getClientIp(clientSocket);
    if (ip.empty()) {
        cout << "Failed to retrieve client address. Error: " << WSAGetLastError() << '\n';
        closesocket(clientSocket);
        return;
    }

    char buffer[1024];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived <= 0) {
        cout << "Failed to receive username from the client.\n";
        closesocket(clientSocket);
        return;
    }

    buffer[bytesReceived] = '\0';
    const string username(buffer);

    if (std::ranges::find(bans, ip) != bans.end()) {
        send(clientSocket, BANNED, static_cast<int>(strlen(BANNED)), 0);
        closesocket(clientSocket);
        cout << "Banned IP tried to connect: " << ip << " " << username << " (" << clientSocket << ")\n";
        return;
    } {
        std::lock_guard lock(clientsMutex);
        clients.emplace_back(clientSocket, username, ip);
    }

    cout << "Client connected: " << ip << " " << username << " (" << clientSocket << ")\n";

    while (true) {
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            std::lock_guard lock(clientsMutex);
            string currentUsername = username;
            for (const auto &client: clients) {
                if (client.getSocket() == clientSocket) {
                    currentUsername = client.getUsername();
                    break;
                }
            }
            cout << "Client disconnected: " << ip << " " << currentUsername << " (" << clientSocket << ")\n";
            break;
        }

        buffer[bytesReceived] = '\0';
        string message(buffer);

        if (message.rfind("/rename ", 0) == 0) {
            string newUsername = message.substr(8);
            string broadcastMsg; {
                std::lock_guard lock(clientsMutex);
                for (auto &client: clients) {
                    if (client.getSocket() == clientSocket) {
                        cout << "Client renamed: " << client.getInfo() << " -> " << newUsername << '\n';
                        broadcastMsg = client.getUsername() + " has changed their name to " + newUsername;
                        client.setUsername(newUsername);
                        break;
                    }
                }
            }

            if (!broadcastMsg.empty()) broadcastMessage(broadcastMsg, clientSocket);
            continue;
        }

        if (message == "/list") {
            std::lock_guard lock(clientsMutex);
            string userList = "Connected users:\n";
            for (const auto &client: clients)
                userList += client.getUsername() + " (" + std::to_string(client.getSocket()) + ")\n";
            send(clientSocket, userList.c_str(), static_cast<int>(userList.size()), 0);
            continue;
        }

        if (message.rfind("/private ", 0) == 0) {
            size_t firstSpace = message.find(' ', 9);
            if (firstSpace == std::string::npos) {
                send(clientSocket, "Usage: /private [socket] [message]\n", 36, 0);
                continue;
            }

            std::string socketStr = message.substr(9, firstSpace - 9);
            std::string privateMessage = message.substr(firstSpace + 1);

            SOCKET targetSocket = static_cast<SOCKET>(std::stoi(socketStr));

            bool messageSent = false; {
                std::lock_guard lock(clientsMutex);
                for (const auto &client: clients) {
                    if (client.getSocket() == targetSocket) {
                        std::string senderUsername;
                        for (const auto &sender: clients) {
                            if (sender.getSocket() == clientSocket) {
                                senderUsername = sender.getUsername();
                                break;
                            }
                        }
                        std::string formattedMessage = "[Private from " + senderUsername + "] " + privateMessage;
                        if (send(targetSocket, formattedMessage.c_str(), static_cast<int>(formattedMessage.size()),
                                 0) != SOCKET_ERROR)
                            messageSent = true;
                        break;
                    }
                }
            }

            if (!messageSent) send(clientSocket, "Error: Target socket not found or message delivery failed.\n", 55, 0);
            continue;
        }

        string currentUsername = username; {
            std::lock_guard lock(clientsMutex);
            for (const auto &client: clients) {
                if (client.getSocket() == clientSocket) {
                    currentUsername = client.getUsername();
                    break;
                }
            }
        }

        broadcastMessage(currentUsername + ": " + message, clientSocket);
    } {
        std::lock_guard lock(clientsMutex);
        string currentUsername = username;
        for (const auto &client: clients) {
            if (client.getSocket() == clientSocket) {
                currentUsername = client.getUsername();
                break;
            }
        }
        removeClient(ClientHandler(clientSocket, currentUsername, ip));
    }
}

void ChatServer::loadBans() {
    bans.clear();
    std::ifstream bansInput(BANS_FILE);
    if (!bansInput) {
        std::ofstream bansOutput(BANS_FILE);
        bansOutput.close();
        cout << "Bans file not found. Created a new file.\n";
    } else {
        std::string line;
        cout << "Loading banned IPs...\n";
        while (getline(bansInput, line)) bans.push_back(line);
        cout << bans.size() << " bans loaded.\n";
    }
    bansInput.close();
}

void ChatServer::broadcastMessage(const std::string &message, const SOCKET senderSocket) {
    std::lock_guard lock(clientsMutex);
    for (auto &client: clients)
        if (client.getSocket() != senderSocket)
            if (send(client.getSocket(), message.c_str(), static_cast<int>(message.size()), 0) == SOCKET_ERROR)
                cout << "Send failed to client " << client.getInfo() << ". Error: " << WSAGetLastError() << '\n';
}

void ChatServer::processServerCommands() {
    Commands commands;

    commands.addCommand("help", "Display available commands", [this](const std::vector<std::string> &) {
        cout << "Server commands:\n";
        cout << "list           - List connected clients\n";
        cout << "bans           - List banned IPs\n";
        cout << "bans refresh   - Reload banned IPs from file\n";
        cout << "bans clear     - Clear all bans\n";
        cout << "ban [IP]       - Ban a specific IP\n";
        cout << "unban [IP]     - Unban a specific IP\n";
        cout << "kick [IP]      - Disconnect a client\n";
        cout << "exit           - Shut down the server\n";
    });

    commands.addCommand("list", "List connected clients", [this](const std::vector<std::string> &) {
        std::lock_guard lock(clientsMutex);
        for (const auto &client: clients) cout << client.getInfo() << '\n';
        cout << "- end (" << clients.size() << ") -\n";
    });

    commands.addCommand("bans", "List banned IPs", [this](const std::vector<std::string> &args) {
        std::lock_guard lock(clientsMutex);
        if (args.empty()) {
            for (const auto &ip: bans) cout << ip << '\n';
            cout << "- end (" << bans.size() << ") -\n";
        } else if (args[0] == "refresh") {
            loadBans();
            for (const ClientHandler &client: clients)
                if (std::ranges::find(bans, client.getIp()) != bans.end()) client.sendBanMessage();
        } else if (args[0] == "clear") {
            bans.clear();

            cout << "Removing all bans...\n";
            std::ofstream bansOutput(BANS_FILE, std::ios::trunc);
            bansOutput.close();
            cout << "Bans removed.\n";
        }
    });

    commands.addCommand("ban", "Ban a specific IP", [this](const std::vector<std::string> &args) {
        if (args.empty()) {
            cout << "Usage: ban [IP]\n";
            return;
        }

        std::lock_guard lock(clientsMutex);
        const string ipToBan = args[0];
        bans.push_back(ipToBan);

        std::ofstream bansOutput(BANS_FILE, std::ios::app);
        bansOutput << ipToBan << '\n';
        bansOutput.close();

        for (const ClientHandler &client: clients) {
            if (ipToBan == client.getIp()) client.sendBanMessage();
        }

        cout << "IP " << ipToBan << " has been banned.\n";
    });

    commands.addCommand("unban", "Unban a specific IP", [this](const std::vector<std::string> &args) {
        if (args.empty()) {
            cout << "Usage: unban [IP]\n";
            return;
        }

        std::lock_guard lock(clientsMutex);
        const string ipToUnban = args[0];

        const auto it = std::remove(bans.begin(), bans.end(), ipToUnban);
        if (it == bans.end()) {
            cout << "IP " << ipToUnban << " is not banned.\n";
            return;
        }

        bans.erase(it, bans.end());

        std::ofstream bansOutput(BANS_FILE, std::ios::trunc);
        for (const string &ip: bans) bansOutput << ip << '\n';
        bansOutput.close();

        cout << "IP " << ipToUnban << " has been unbanned.\n";
    });

    commands.addCommand("kick", "Disconnect a client", [this](const std::vector<std::string> &args) {
        if (args.empty()) {
            cout << "Usage: kick [IP]\n";
            return;
        }

        std::lock_guard lock(clientsMutex);
        const string ipToKick = args[0];
        bool kicked = false;

        for (const ClientHandler &client: clients)
            if (ipToKick == client.getIp()) {
                kicked = true;
                client.sendKickMessage();
            }

        if (kicked) cout << "IP " << ipToKick << " has been kicked.\n";
        else cout << "No client connected by the IP " << ipToKick << ".\n";
    });

    commands.addCommand("exit", "Shut down the server", [this](const std::vector<std::string> &) {
        std::lock_guard lock(clientsMutex);
        shutdown();
    });

    string input;
    while (true) {
        getline(std::cin, input);
        commands.executeCommand(input);
        cout << '\n';
    }

    std::lock_guard lock(clientsMutex);
    for (const ClientHandler &client: clients) closesocket(client.getSocket());

    shutdown();
}

std::string ChatServer::getClientIp(const SOCKET clientSocket) {
    sockaddr_in clientAddress;
    int clientAddressSize = sizeof(clientAddress);
    if (getpeername(clientSocket, reinterpret_cast<sockaddr *>(&clientAddress), &clientAddressSize) == 0) {
        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddress.sin_addr, clientIp, INET_ADDRSTRLEN);
        return clientIp;
    }

    return "";
}

void ChatServer::removeClient(const ClientHandler &client) {
    if (const auto it = std::ranges::find_if(clients, [&client](const ClientHandler &cl) {
        return cl.getSocket() == client.getSocket();
    }); it != clients.end())
        clients.erase(it);
}
