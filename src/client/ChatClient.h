#pragma once

#include <string>
#include <mutex>
#include <winsock2.h>

class ChatClient {
public:
    explicit ChatClient(const std::string &, const unsigned short &, const std::string &);

    ~ChatClient();

    void run();

    void shutdown() const;

private:
    std::string ip;
    unsigned short port;
    std::string username;
    SOCKET clientSocket;
    std::mutex outputMutex;

    void setupConnection();

    void handleSending() const;

    void handleReceiving();

    static void removeLastLine();

    static void printMessage(const std::string &);
};
