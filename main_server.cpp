#include <iostream>
#include <string>

#include "src/common/Utils.h"
#include "src/server/ChatServer.h"

using std::string;
using std::cout;
using std::cin;
using std::getline;

string input;
unsigned short port = 8080;

void initializePort();

int main() {
    initializePort();

    try {
        ChatServer(port).run();
    } catch (const std::runtime_error &e) {
        cout << e.what() << '\n';
        cin.get();
        return 1;
    }

    return 0;
}

void initializePort() {
    cout << "Choose port (default " << port << "): ";
    Utils().enterPort(input, port);
}
