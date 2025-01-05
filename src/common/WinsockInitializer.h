#pragma once

#include <winsock2.h>

class WinsockInitializer {
public:
    WinsockInitializer();

    ~WinsockInitializer();

private:
    WSADATA wsaData{};
};
