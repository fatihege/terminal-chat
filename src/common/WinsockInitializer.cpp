#include "WinsockInitializer.h"

#include <stdexcept>
#include <string>
#include <winsock2.h>

WinsockInitializer::WinsockInitializer() {
    if (const int result = WSAStartup(MAKEWORD(2, 2), &wsaData); result != 0)
        throw std::runtime_error("WSAStartup failed. Error: " + std::to_string(result));
}

WinsockInitializer::~WinsockInitializer() {
    WSACleanup();
}
