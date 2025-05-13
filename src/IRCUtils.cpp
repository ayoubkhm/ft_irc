#include "IRCUtils.hpp"
#include <iostream>
#include <cstring>
#include <cerrno>
#include <unistd.h>

void sendResponse(Client* client, const std::string &response)
{
    std::string resp = response + "\r\n";
    if (write(client->getFd(), resp.c_str(), resp.size()) < 0)
    {
        std::cerr << "Erreur lors de l'envoi de réponse: " << strerror(errno) << "\n";
    }
}
