#include <iostream>
#include <mutex>
#include <string>
#include <winsock2.h>

#include "src/client/ChatClient.h"
#include "src/common/Utils.h"

using std::string;
using std::cout;
using std::cin;
using std::getline;

int initialize(string &, string &, string &, unsigned short &);

int main() {
    string input;
    string username;
    string ip = "127.0.0.1";
    unsigned short port = 8080;

    if (const int result = initialize(input, username, ip, port); result != 0) return result;

    try {
        ChatClient(ip, port, username).run();
    } catch (const std::runtime_error &e) {
        cout << e.what() << '\n';
        cin.get();
        return 1;
    }

    return 0;
}

int initialize(string &input, string &username, string &ip, unsigned short &port) {
    cout << "Enter username: ";
    getline(cin, input);
    if (!input.empty()) username = input;

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
    Utils().enterPort(input, port);

    return 0;
}
