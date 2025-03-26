#pragma once

#include <string>
#include <vector>
#include "Client.hpp"
#include "Server.hpp"
#include <iostream>
#include <cctype>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <cstdlib>

// Parse le message reçu et dispatch la commande correspondante
void parseAndDispatch(Server* server, Client* client, const std::string &message);

