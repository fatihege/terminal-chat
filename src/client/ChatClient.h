#pragma once

#include <string>
#include <mutex>
#include <winsock2.h>

class ChatClient {
public:
    explicit ChatClient(std::string , const unsigned short &, std::string );

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

    void handleSending();

    void handleReceiving();

    static void removeLastLine();

    static void printMessage(const std::string &);
};
