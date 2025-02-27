#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <string>

// Parse le message reçu et dispatch la commande correspondante
void parseAndDispatch(int client_fd, const std::string &message);

#endif
