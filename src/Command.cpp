#include "Command.hpp"
#include <iostream>
#include <vector>
#include <cctype>
#include <cstring>
#include <cerrno>
#include <unistd.h>

// Fonction de Test pour gérer la commande PING en répondant par PONG
// En gros tu envoie PING, il répond PONG lolilol
void handlePing(int client_fd, const std::vector<std::string>& params)
{
    (void)params;
    const char *reply = "PONG\r\n";
    if (write(client_fd, reply, std::strlen(reply)) < 0)
        std::cerr << "Erreur lors de l'envoi de PONG: " << strerror(errno) << "\n";
    else
        std::cout << "[PING] Envoyé PONG à FD " << client_fd << "\n";
}

// Dispatch de commande selon le premier token
void dispatchCommand(int client_fd, const std::vector<std::string>& tokens)
{
    if (tokens.empty())
        return;
    std::string cmd = tokens[0];
    for (size_t i = 0; i < cmd.size(); ++i)
        cmd[i] = std::toupper(cmd[i]);
    std::vector<std::string> params(tokens.begin() + 1, tokens.end());
    if (cmd == "PING")
        handlePing(client_fd, params);
    else
        std::cout << "[UNKNOWN] Commande inconnue: " << tokens[0] << "\n";
}

// Découpe le message en tokens et dispatch la commande
void parseAndDispatch(int client_fd, const std::string &message) 
{
    std::vector<std::string> tokens;
    size_t pos = 0;
    while (pos < message.size())
    {
        while (pos < message.size() && std::isspace(message[pos]))
            pos++;
        if (pos >= message.size())
            break;
        if (message[pos] == ':')
        {
            tokens.push_back(message.substr(pos + 1));
            break;
        }
        size_t end = pos;
        while (end < message.size() && !std::isspace(message[end]))
            end++;
        tokens.push_back(message.substr(pos, end - pos));
        pos = end;
    }
    if (tokens.empty())
    {
        std::cout << "Commande vide reçue.\n";
        return;
    }
    std::cout << "Commande détectée: " << tokens[0] << "\n";
    dispatchCommand(client_fd, tokens);
}
