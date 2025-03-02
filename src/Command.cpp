#include "Command.hpp"
#include <iostream>
#include <vector>
#include <cctype>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <cstdlib>

Channel* Server::getChannelByName(const std::string& channelName)
{
    std::map<std::string, Channel>::iterator it = _Channels.find(channelName);
    if (it != _Channels.end())
    {
        return &it->second;
    }
    return NULL;
}

// Fonction utilitaire pour envoyer une réponse au client (ajoute CRLF)
static void sendResponse(Client* client, const std::string &response)
{
    std::string resp = response + "\r\n";
    if (write(client->getFd(), resp.c_str(), resp.size()) < 0)
    {
        std::cerr << "Erreur lors de l'envoi de réponse: " << strerror(errno) << "\n";
    }
}

// Handler pour la commande PASS
void handlePass(Client* client, const std::vector<std::string>& params)
{
    if (params.empty())
    {
        sendResponse(client, "Erreur: PASS nécessite un argument");
        return;
    }
    client->authenticate(params[0]);
    if (client->isAuthenticated())
    {
        sendResponse(client, "Client authentifié via PASS");
    }
    else
    {
        sendResponse(client, "Mot de passe incorrect");
    }
}

// Handler pour la commande NICK
void handleNick(Client* client, const std::vector<std::string>& params)
{
    if (params.empty())
    {
        sendResponse(client, "Erreur: NICK nécessite un argument");
        return;
    }
    client->setNickname(const_cast<std::string&>(params[0]));
    sendResponse(client, "Nickname défini: " + params[0]);
    if (client->isAuthenticated() && !client->getNickname().empty() && !client->getUsername().empty())
    {
        sendResponse(client, "Client enregistré");
    }
}

// Handler pour la commande USER
void handleUser(Client* client, const std::vector<std::string>& params)
{
    if (params.size() < 2)
    {
        sendResponse(client, "Erreur: USER nécessite au moins 2 arguments");
        return;
    }
    client->setUsername(const_cast<std::string&>(params[0]));
    sendResponse(client, "Username défini: " + params[0]);
    if (client->isAuthenticated() && !client->getNickname().empty() && !client->getUsername().empty())
    {
        sendResponse(client, "Client enregistré");
    }
}

// Handler pour la commande JOIN
void handleJoin(Server* server, Client* client, const std::vector<std::string>& params)
{
    if (params.empty())
    {
        sendResponse(client, "Erreur: JOIN nécessite un nom de channel");
        return;
    }
    
    // Ajouter le client au channel
    server->joinChannel(client->getFd(), params[0]);
    
    // Vérifier que le client est bien dans le channel
    Channel* channel = server->getChannelByName(params[0]);
    if (channel && channel->isClientInChannel(client->getFd()))
    {
        sendResponse(client, "JOIN " + params[0] + " réussi");
        
        // Notifier tous les membres du channel que ce client vient de rejoindre
        std::string joinMsg = ":" + client->getNickname() + " JOIN " + params[0];
        server->broadcastToChannel(params[0], joinMsg, client->getFd());
    }
    else
    {
        sendResponse(client, "Erreur lors de l'inscription au channel " + params[0]);
    }
}


// Handler pour la commande PRIVMSG
void handlePrivmsg(Server* server, Client* client, const std::vector<std::string>& params)
{
    if (params.size() < 2)
    {
        sendResponse(client, "Erreur: PRIVMSG nécessite une cible et un message");
        return;
    }
    std::string target = params[0];
    std::string message = "";
    size_t i = 1;
    while (i < params.size())
    {
        if (i > 1)
        {
            message += " ";
        }
        message += params[i];
        i++;
    }
    if (!target.empty() && target[0] == '#')
    {
        server->broadcastToChannel(target, "PRIVMSG " + target + " :" + message, client->getFd());
        sendResponse(client, "Message envoyé au channel " + target);
    }
    else
    {
        sendResponse(client, "Erreur: PRIVMSG vers un utilisateur n'est pas implémenté");
    }
}

// Handler pour la commande KICK
void handleKick(Server* server, Client* client, const std::vector<std::string>& params)
{
    if (params.size() < 2)
    {
        sendResponse(client, "Erreur: KICK nécessite un nom de channel et une cible");
        return;
    }
    int targetFd = server->getFdByNickname(params[1]);
    if (targetFd == -1)
    {
        sendResponse(client, "Erreur: le pseudo " + params[1] + " n'a pas été trouvé.");
        return;
    }
    // Vérifier que l'opérateur ne tente pas de se kicker lui-même
    if (targetFd == client->getFd())
    {
        sendResponse(client, "Erreur: Vous ne pouvez pas vous kick vous-même !");
        return;
    }
    
    // Récupérer le channel correspondant
    Channel* channel = server->getChannelByName(params[0]);
    if (!channel)
    {
        sendResponse(client, "Erreur: Le channel " + params[0] + " n'existe pas.");
        return;
    }
    
    // Vérifier que le client ciblé est bien dans le channel
    if (!channel->isClientInChannel(targetFd))
    {
        std::cout << "DEBUG: Le client avec fd " << targetFd 
                  << " n'est pas dans le channel " << params[0] << std::endl;
        sendResponse(client, "Erreur: Le client " + params[1] + " n'est pas dans le channel " + params[0] + ".");
        return;
    }
    
    // On peut procéder au kick
    std::cout << "DEBUG: Tentative de kick du client fd " << targetFd 
              << " du channel " << params[0] << std::endl;
    server->kickClient(client->getFd(), params[0], targetFd);
    std::cout << "DEBUG: Client avec fd " << targetFd 
              << " a été expulsé du channel " << params[0] << "." << std::endl;
    sendResponse(client, "KICK " + params[0] + " " + params[1] + " effectué");

    
    // Notifier le client kické
    Client* kickedClient = server->getClientByFd(targetFd);
    if (kickedClient)
    {
        sendResponse(kickedClient, "Vous avez été kick du channel " + params[0]);
        std::cout << "DEBUG: Notification envoyée au client fd " << targetFd << std::endl;
    }
}

// Handler pour la commande INVITE
void handleInvite(Server* server, Client* client, const std::vector<std::string>& params)
{
    if (params.size() < 2)
    {
        sendResponse(client, "Erreur: INVITE nécessite un nom de channel et une cible");
        return;
    }
    
    // Récupérer le fd du client à inviter
    int targetFd = server->getFdByNickname(params[1]);
    if (targetFd == -1)
    {
        sendResponse(client, "Erreur: le pseudo " + params[1] + " n'a pas été trouvé.");
        return;
    }
    
    // Vérifier que le client ne tente pas de s'inviter lui-même
    if (targetFd == client->getFd())
    {
        sendResponse(client, "Erreur: Vous ne pouvez pas vous inviter vous-même !");
        return;
    }
    
    // Récupérer le channel concerné
    Channel* channel = server->getChannelByName(params[0]);
    if (!channel)
    {
        sendResponse(client, "Erreur: Le channel " + params[0] + " n'existe pas.");
        return;
    }
    
    // Vérifier que le client à inviter n'est pas déjà dans le channel
    if (channel->isClientInChannel(targetFd))
    {
        sendResponse(client, "Erreur: Le client " + params[1] + " est déjà dans le channel " + params[0] + ".");
        return;
    }
    
    // Debug : affichage de la tentative d'invitation
    std::cout << "DEBUG: Tentative d'inviter le client fd " << targetFd 
              << " au channel " << params[0] << std::endl;
    
    // Récupérer le pointeur du client à inviter
    Client* invitedClient = server->getClientByFd(targetFd);
    if (!invitedClient)
    {
        sendResponse(client, "Erreur: Impossible d'inviter " + params[1]);
        return;
    }
    
    // Envoyer la notification d'invitation au client ciblé
    sendResponse(invitedClient, "Vous avez été invité à rejoindre le channel " + params[0] +
                                  " par " + client->getNickname());
    std::cout << "DEBUG: Invitation envoyée au client fd " << targetFd 
              << " pour le channel " << params[0] << std::endl;
    
    // Ajouter effectivement le client dans le channel
    server->joinChannel(targetFd, params[0]);
    std::cout << "DEBUG: Le client avec fd " << targetFd 
              << " a rejoint le channel " << params[0] << " suite à l'invitation." << std::endl;
    
    // Notifier l'émetteur que l'invitation a bien été envoyée et le client ajouté
    sendResponse(client, "INVITE " + params[0] + " " + params[1] +
                          " envoyé et " + params[1] + " ajouté au channel.");
}

// Handler pour la commande TOPIC (dummy)
void handleTopic(Server* server, Client* client, const std::vector<std::string>& params)
{
    (void) server;
    (void) params;
    sendResponse(client, "TOPIC non implémentée");
}

// Handler pour la commande MODE (dummy)
void handleMode(Server* server, Client* client, const std::vector<std::string>& params)
{
    (void) server;
    (void) params;
    sendResponse(client, "MODE non implémentée");
}

void dispatchCommand(Server* server, Client* client, const std::vector<std::string>& tokens)
{
    if (tokens.empty())
    {
        return;
    }
    std::string cmd = tokens[0];
    size_t i = 0;
    while (i < cmd.size())
    {
        cmd[i] = std::toupper(cmd[i]);
        i++;
    }
    std::vector<std::string> params(tokens.begin() + 1, tokens.end());
    
    bool registered = client->isAuthenticated() &&
                      (!client->getNickname().empty()) &&
                      (!client->getUsername().empty());
    
    if (!registered)
    {
        if (cmd != "PASS" && cmd != "NICK" && cmd != "USER")
        {
            sendResponse(client, "Erreur: Vous devez vous enregistrer (PASS, NICK, USER) avant d'utiliser d'autres commandes.");
            return;
        }
    }
    
    if (cmd == "PASS")
    {
        handlePass(client, params);
    }
    else if (cmd == "NICK")
    {
        handleNick(client, params);
    }
    else if (cmd == "USER")
    {
        handleUser(client, params);
    }
    else if (cmd == "JOIN")
    {
        handleJoin(server, client, params);
    }
    else if (cmd == "PRIVMSG")
    {
        handlePrivmsg(server, client, params);
    }
    else if (cmd == "KICK")
    {
        handleKick(server, client, params);
    }
    else if (cmd == "INVITE")
    {
        handleInvite(server, client, params);
    }
    else if (cmd == "TOPIC")
    {
        handleTopic(server, client, params);
    }
    else if (cmd == "MODE")
    {
        handleMode(server, client, params);
    }
    else
    {
        sendResponse(client, "[UNKNOWN] Commande inconnue: " + tokens[0]);
    }
}

void parseAndDispatch(Server* server, Client* client, const std::string &message)
{
    std::vector<std::string> tokens;
    size_t pos = 0;
    while (pos < message.size())
    {
        while (pos < message.size() && std::isspace(message[pos]))
        {
            pos++;
        }
        if (pos >= message.size())
        {
            break;
        }
        if (message[pos] == ':')
        {
            tokens.push_back(message.substr(pos + 1));
            break;
        }
        size_t end = pos;
        while (end < message.size() && !std::isspace(message[end]))
        {
            end++;
        }
        tokens.push_back(message.substr(pos, end - pos));
        pos = end;
    }
    if (tokens.empty())
    {
        sendResponse(client, "Commande vide reçue.");
        return;
    }
    std::cout << "Commande détectée: " << tokens[0] << "\n";
    dispatchCommand(server, client, tokens);
}
