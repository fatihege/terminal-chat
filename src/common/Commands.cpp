#include "Commands.h"

#include <iostream>
#include <sstream>

using std::cout;
using std::string;

Commands::Commands(const string &prefix): prefix(prefix) {
}

void Commands::addCommand(const string &name, const string &description, CommandHandler handler) {
    commands[name] = {description, handler};
}

void Commands::executeCommand(const string &input) {
    if (!prefix.empty() && input.rfind(prefix, 0) != 0) {
        cout << "Unknown command or missing prefix '" << prefix << "'.\n";
        return;
    }

    const string commandInput = prefix.empty() ? input : input.substr(prefix.size());

    auto args = splitArguments(commandInput);
    if (args.empty()) {
        cout << "Empty command.\n";
        return;
    }

    const string command = args[0];
    args.erase(args.begin());

    if (const auto it = commands.find(command); it != commands.end()) it->second.second(args);
    else cout << "Unknown command: " << command << '\n';
}

void Commands::listCommands() const {
    cout << "Available commands:\n";
    for (const auto &[name, pair]: commands)
        cout << prefix << name << " - " << pair.first << '\n';
}

std::vector<string> Commands::splitArguments(const string &input) {
    std::istringstream iss(input);
    std::vector<string> args;
    string arg;
    while (iss >> arg) args.push_back(arg);
    return args;
}
