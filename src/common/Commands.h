#pragma once

#include <string>
#include <functional>
#include <unordered_map>
#include <vector>

class Commands {
public:
    using CommandHandler = std::function<void(const std::vector<std::string> &args)>;

    explicit Commands(const std::string &prefix = "");

    void addCommand(const std::string &name, const std::string &description, CommandHandler handler);

    void executeCommand(const std::string &input);

    void listCommands() const;

private:
    std::string prefix;
    std::unordered_map<std::string, std::pair<std::string, CommandHandler> > commands;

    static std::vector<std::string> splitArguments(const std::string &input);
};
