#pragma once

#include <string>
#include <vector>
#include "Client.hpp"
#include "Server.hpp"

// Parse le message reçu et dispatch la commande correspondante
void parseAndDispatch(Server* server, Client* client, const std::string &message);

