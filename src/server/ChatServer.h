#pragma once

#define BANS_FILE "bans"

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <winsock2.h>

#include "ClientHandler.h"
// #include "Command.h"

class ChatServer {
public:
    explicit ChatServer(const unsigned short &);

    ~ChatServer();

    void run();

    void shutdown();

private:
    unsigned short port;
    SOCKET serverSocket;
    std::vector<ClientHandler> clients;
    // std::vector<Command> commands;
    std::vector<std::string> bans;
    std::mutex clientsMutex;
    std::atomic<bool> isRunning;

    void initializeSocket();

    void acceptClientConnections();

    void processClientConnection(SOCKET);

    void loadBans();

    void broadcastMessage(const std::string &, SOCKET);

    void processServerCommands();

    static std::string getClientIp(SOCKET);

    void removeClient(const ClientHandler &client);
};
