#pragma once

#include <string>
#include "Client.hpp"

void sendResponse(Client* client, const std::string &response);
void displayRegistrationInstructions(Client *client);
