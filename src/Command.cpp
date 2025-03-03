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

// Handler pour la commande PING
void handlePing(Client* client, const std::vector<std::string>& params)
{
    if (params.empty())
    {
        sendResponse(client, "409 :No origin specified");
        return;
    }
    // Répondre avec PONG suivi du paramètre reçu
    sendResponse(client, "PONG " + params[0]);
}


// Handler pour la commande PASS
void handlePass(Client* client, const std::vector<std::string>& params)
{
    if (params.empty())
    {
        sendResponse(client, "461 PASS :Not enough parameters");
        return;
    }
    client->authenticate(params[0]);
    if (client->isAuthenticated())
    {
        sendResponse(client, "NOTICE :Mot de passe accepté");
    }
    else
    {
        sendResponse(client, "464 PASS :Mot de passe incorrect");
    }
}

// Handler pour la commande NICK
void handleNick(Client* client, const std::vector<std::string>& params)
{
    if (params.empty())
    {
        sendResponse(client, "461 NICK :Not enough parameters");
        return;
    }
    client->setNickname(const_cast<std::string&>(params[0]));
    sendResponse(client, "NICK " + params[0] + " :Nickname set");
    if (client->isAuthenticated() && !client->getNickname().empty() && !client->getUsername().empty())
    {
        sendResponse(client, "001 " + client->getNickname() + " :Bienvenue sur ft_irc");
    }
}

// Handler pour la commande USER
void handleUser(Client* client, const std::vector<std::string>& params)
{
    if (params.size() < 2)
    {
        sendResponse(client, "461 USER :Not enough parameters");
        return;
    }
    client->setUsername(const_cast<std::string&>(params[0]));
    sendResponse(client, "USER " + params[0] + " :Username set");
    if (client->isAuthenticated() && !client->getNickname().empty() && !client->getUsername().empty())
    {
        sendResponse(client, "001 " + client->getNickname() + " :Bienvenue sur ft_irc");
    }
}

// Handler pour la commande JOIN
void handleJoin(Server* server, Client* client, const std::vector<std::string>& params)
{
    if (params.empty())
    {
        sendResponse(client, "461 JOIN :Not enough parameters");
        return;
    }
    
    // Ajouter le client au channel
    server->joinChannel(client->getFd(), params[0]);
    
    // Vérifier que le client est bien dans le channel
    Channel* channel = server->getChannelByName(params[0]);
    if (channel && channel->isClientInChannel(client->getFd()))
    {
        sendResponse(client, "JOIN " + params[0] + " :Join successful");
        
        // Notifier tous les membres du channel que ce client vient de rejoindre
        std::string joinMsg = ":" + client->getNickname() + " JOIN " + params[0];
        server->broadcastToChannel(params[0], joinMsg, client->getFd());
    }
    else
    {
        sendResponse(client, "403 " + params[0] + " :No such channel");
    }
}

// Handler pour la commande PRIVMSG
void handlePrivmsg(Server* server, Client* client, const std::vector<std::string>& params)
{
    if (params.size() < 2)
    {
        sendResponse(client, "461 PRIVMSG :Not enough parameters");
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
        sendResponse(client, "NOTICE PRIVMSG :Message delivered to " + target);
    }
    else
    {
        sendResponse(client, "401 " + target + " :No such nick/channel");
    }
}

// Handler pour la commande KICK
void handleKick(Server* server, Client* client, const std::vector<std::string>& params)
{
    if (params.size() < 2)
    {
        sendResponse(client, "461 KICK :Not enough parameters");
        return;
    }
    int targetFd = server->getFdByNickname(params[1]);
    if (targetFd == -1)
    {
        sendResponse(client, "401 " + params[1] + " :No such nick/channel");
        return;
    }
    // Vérifier que l'opérateur ne tente pas de se kicker lui-même
    if (targetFd == client->getFd())
    {
        sendResponse(client, "502 :Cannot kick yourself");
        return;
    }
    
    // Récupérer le channel correspondant
    Channel* channel = server->getChannelByName(params[0]);
    if (!channel)
    {
        sendResponse(client, "403 " + params[0] + " :No such channel");
        return;
    }
    
    // Vérifier que le client ciblé est bien dans le channel
    if (!channel->isClientInChannel(targetFd))
    {
        std::cout << "DEBUG: Le client avec fd " << targetFd 
                  << " n'est pas dans le channel " << params[0] << std::endl;
        sendResponse(client, "441 " + params[1] + " " + params[0] + " :They aren't on that channel");
        return;
    }
    
    // On peut procéder au kick
    std::cout << "DEBUG: Tentative de kick du client fd " << targetFd 
              << " du channel " << params[0] << std::endl;
    server->kickClient(client->getFd(), params[0], targetFd);
    std::cout << "DEBUG: Client avec fd " << targetFd 
              << " a été expulsé du channel " << params[0] << "." << std::endl;
    sendResponse(client, "KICK " + params[0] + " " + params[1] + " :Kick successful");
    
    // Notifier le client kické
    Client* kickedClient = server->getClientByFd(targetFd);
    if (kickedClient)
    {
        sendResponse(kickedClient, "NOTICE KICK :You have been kicked from " + params[0]);
        std::cout << "DEBUG: Notification envoyée au client fd " << targetFd << std::endl;
    }
}

// Handler pour la commande INVITE
void handleInvite(Server* server, Client* client, const std::vector<std::string>& params)
{
    if (params.size() < 2)
    {
        sendResponse(client, "461 INVITE :Not enough parameters");
        return;
    }
    
    // Récupérer le fd du client à inviter
    int targetFd = server->getFdByNickname(params[1]);
    if (targetFd == -1)
    {
        sendResponse(client, "401 " + params[1] + " :No such nick/channel");
        return;
    }
    
    // Vérifier que le client ne tente pas de s'inviter lui-même
    if (targetFd == client->getFd())
    {
        sendResponse(client, "502 :Cannot invite yourself");
        return;
    }
    
    // Récupérer le channel concerné
    Channel* channel = server->getChannelByName(params[0]);
    if (!channel)
    {
        sendResponse(client, "403 " + params[0] + " :No such channel");
        return;
    }
    
    // Vérifier que le client à inviter n'est pas déjà dans le channel
    if (channel->isClientInChannel(targetFd))
    {
        sendResponse(client, "443 " + params[1] + " " + params[0] + " :is already on that channel");
        return;
    }
    
    // Debug : affichage de la tentative d'invitation
    std::cout << "DEBUG: Tentative d'inviter le client fd " << targetFd 
              << " au channel " << params[0] << std::endl;
    
    // Récupérer le pointeur du client à inviter
    Client* invitedClient = server->getClientByFd(targetFd);
    if (!invitedClient)
    {
        sendResponse(client, "401 " + params[1] + " :No such nick/channel");
        return;
    }
    
    // Envoyer la notification d'invitation au client ciblé
    sendResponse(invitedClient, "341 " + params[0] + " " + params[1] + " :You have been invited by " + client->getNickname());
    std::cout << "DEBUG: Invitation envoyée au client fd " << targetFd 
              << " pour le channel " << params[0] << std::endl;
    
    // Ajouter effectivement le client dans le channel
    server->joinChannel(targetFd, params[0]);
    std::cout << "DEBUG: Le client avec fd " << targetFd 
              << " a rejoint le channel " << params[0] << " suite à l'invitation." << std::endl;
    
    // Notifier l'émetteur que l'invitation a bien été envoyée et le client ajouté
    sendResponse(client, "341 " + params[0] + " " + params[1] + " :Invite successful and user added");
}

// Handler pour la commande TOPIC (dummy)
void handleTopic(Server* server, Client* client, const std::vector<std::string>& params)
{
    (void) server;
    (void) params;
    sendResponse(client, "332 :TOPIC non implémentée");
}

// Handler pour la commande MODE (dummy)
void handleMode(Server* server, Client* client, const std::vector<std::string>& params)
{
    (void) server;
    (void) params;
    sendResponse(client, "221 :MODE non implémentée");
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
            sendResponse(client, "451 :You have not registered");
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
        if (client->isAuthenticated() &&
            !client->getNickname().empty() &&
            !client->getUsername().empty() &&
            !client->hasReceivedWelcome())
        {
            sendResponse(client, "001 " + client->getNickname() + " :Bienvenue sur ft_irc");
            client->setWelcomeReceived(true);
        }
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
    else if (cmd == "PING")
    {
        handlePing(client, params);
    }
    else
    {
        sendResponse(client, "421 " + tokens[0] + " :Unknown command");
    }
}



void parseAndDispatch(Server* server, Client* client, const std::string &message)
{
    // Découper le buffer en lignes, en se basant sur "\r\n"
    size_t start = 0;
    size_t end = message.find("\r\n");
    while (end != std::string::npos)
    {
        std::string line = message.substr(start, end - start);
        if (!line.empty())
        {
            std::vector<std::string> tokens;
            size_t pos = 0;
            while (pos < line.size())
            {
                // Passer les espaces
                while (pos < line.size() && std::isspace(line[pos]))
                    pos++;
                if (pos >= line.size())
                    break;
                if (line[pos] == ':')
                {
                    tokens.push_back(line.substr(pos + 1));
                    break;
                }
                size_t space = pos;
                while (space < line.size() && !std::isspace(line[space]))
                    space++;
                tokens.push_back(line.substr(pos, space - pos));
                pos = space;
            }
            if (tokens.empty())
            {
                sendResponse(client, "421 :Empty command received");
            }
            else
            {
                std::cout << "Commande détectée: " << tokens[0] << "\n";
                dispatchCommand(server, client, tokens);
            }
        }
        start = end + 2; // Passer "\r\n"
        end = message.find("\r\n", start);
    }
    
    // Si il reste une ligne sans CRLF à la fin
    if (start < message.size())
    {
        std::string line = message.substr(start);
        if (!line.empty())
        {
            std::vector<std::string> tokens;
            size_t pos = 0;
            while (pos < line.size())
            {
                while (pos < line.size() && std::isspace(line[pos]))
                    pos++;
                if (pos >= line.size())
                    break;
                if (line[pos] == ':')
                {
                    tokens.push_back(line.substr(pos + 1));
                    break;
                }
                size_t space = pos;
                while (space < line.size() && !std::isspace(line[space]))
                    space++;
                tokens.push_back(line.substr(pos, space - pos));
                pos = space;
            }
            if (!tokens.empty())
            {
                std::cout << "Commande détectée: " << tokens[0] << "\n";
                dispatchCommand(server, client, tokens);
            }
        }
    }
}
