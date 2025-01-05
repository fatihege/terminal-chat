#include <iostream>
#include <string>

#include "Utils.h"

void Utils::enterPort(std::string &input, unsigned short &port) {
    std::getline(std::cin, input);
    if (!input.empty()) {
        try {
            if (const unsigned long newPort = std::stoul(input); newPort > 0 && newPort <= 65535)
                port = static_cast<unsigned short>(newPort);
            else std::cout << "Invalid port range. Using default port.\n";
        } catch (const std::exception &ex) {
            std::cout << "Invalid input (" << ex.what() << "). Using default port.\n";
        }
    }
}
