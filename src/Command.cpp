// Command.cpp

#include "Command.hpp"
#include "IRCUtils.hpp" // Déclare sendResponse(...)
#include "Server.hpp"
#include "RPL.hpp"
#include <iostream>
#include <vector>
#include <cctype>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <cstdlib>

// (La définition de Server::getChannelByName a été supprimée ici car elle est définie dans Server.cpp)

// Handler pour la commande CAP
void handleCap(Client* client, const std::vector<std::string>& params)
{
    (void)params; // Paramètre inutilisé
    sendResponse(client, "CAP * LS :multi-prefix");
}

// Handler pour la commande PING
void handlePing(Client* client, const std::vector<std::string>& params)
{
    if (params.empty())
    {
        sendResponse(client, "409 :No origin specified");
        return;
    }
    sendResponse(client, RPL_PONG(user_id(client->getNickname(), client->getUsername()), params[0]));
}

// Handler pour la commande PASS
bool handlePass(Client* client, const std::vector<std::string>& params)
{
    if (params.empty())
    {
        sendResponse(client, "461 PASS :Not enough parameters");
        return false;
    }
    client->authenticate(params[0]);
    if (client->isAuthenticated()){
        sendResponse(client, "NOTICE :Mot de passe accepté");
        return true;
    }
    else {
        sendResponse(client, "464 PASS :Mot de passe incorrect");
        return false;
    }
}

// Handler pour la commande NICK
bool handleNick(Server* server, Client* client, const std::vector<std::string>& params)
{
    if (params.empty())
    {
        sendResponse(client, "461 NICK :Not enough parameters");
        return false;
    }
    std::string nickName = params[0];
    if (server->checkDuplicateClient(nickName))
    {
        sendResponse(client, "461 NICK : " + nickName + " Duplicate NICK");
        return false;
    }
    client->setNickname(const_cast<std::string&>(params[0]));
    return true;
    // Pas d'écho welcome ici pour éviter les doublons.
}

// Handler pour la commande USER
bool handleUser(Client* client, const std::vector<std::string>& params)
{
    if (params.size() < 1)
    {
        sendResponse(client, "461 USER :Not enough parameters");
        return false;
    }
    client->setUsername(const_cast<std::string&>(params[0]));
    return true;
    // Le welcome sera envoyé dans dispatchCommand.
}

// Handler pour la commande JOIN
void handleJoin(Server* server, Client* client, const std::vector<std::string>& params)
{
    if (params.empty())
    {
        sendResponse(client, ":ft_irc 461 JOIN :Not enough parameters");
        return;
    }
    std::string channelName = params[0];
    if (channelName.empty() || channelName[0] != '#')
    {
        sendResponse(client, ":ft_irc 403 " + channelName + " :Invalid channel name (must begin with '#')");
        return;
    }
    
    Channel* channel = server->getChannelByName(channelName);
    if (channel)
    {
        // Vérification par ID unique
        if (channel->isClientInChannel(client->getId()))
        {
            sendResponse(client, ":ft_irc 473 " + channelName + " :Cannot join channel already in");
            return;
        }
        if (channel->getInviteOnly() && !channel->isClientInvited(client->getId()))
        {
            sendResponse(client, ":ft_irc 473 " + channelName + " :Cannot join channel (+i)");
            return;
        }
        if (!channel->getKey().empty())
        {
            if (params.size() < 2)
            {
                sendResponse(client, ":ft_irc 475 " + channelName + " :Cannot join channel (+k) - key missing");
                return;
            }
            std::string mdp = params[1];
            if (channel->getKey() != mdp)
            {
                sendResponse(client, ":ft_irc 475 " + channelName + " :Cannot join channel (+k) - bad key");
                return;
            }
        }
    }
    
    // Ajout du client dans le channel
    // On laisse le serveur gérer la création du channel via le FD, puis il utilisera getId() en interne.
    server->joinChannel(client->getFd(), channelName);
    channel = server->getChannelByName(channelName);
    
    if (channel && channel->isClientInChannel(client->getId()))
    {
        // Retirer le client de la liste des invités
        channel->removeInvitedClient(client->getId());
        // Message JOIN complet
        std::string joinMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost JOIN " + channelName;
        server->broadcastToChannel(channelName, joinMsg, -1);
        sendResponse(client, ":ft_irc 001 " + client->getNickname() + " :Joined channel " + channelName);
    }
    else
    {
        sendResponse(client, ":ft_irc 403 " + channelName + " :No such channel");
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
    std::string message;
    for (size_t i = 1; i < params.size(); i++)
    {
        if (i > 1)
            message += " ";
        message += params[i];
    }
    
    // Message vers un channel
    if (!target.empty() && target[0] == '#')
    {
        Channel* channel = server->getChannelByName(target);
        if (channel == NULL || !channel->isClientInChannel(client->getId()))
        {
            sendResponse(client, "442 " + target + " :You're not on that channel");
            return;
        }
        std::string msg = ":" + client->getNickname() + " PRIVMSG " + target + " :" + message;
        server->broadcastToChannel(target, msg, client->getFd());
        sendResponse(client, "NOTICE PRIVMSG :Message delivered to " + target);
    }
    // Message privé
    else if (!target.empty() && target[0] != '#' && target != client->getNickname())
    {
        int targetFd = server->getFdByNickname(target);
        if (targetFd == -1)
        {
            sendResponse(client, "401 " + target + " :No such nick/channel");
            return;
        }
        Client* targetClient = server->getClientByFd(targetFd);
        if (!targetClient)
        {
            sendResponse(client, "402 " + target + " :No such nick/channel");
            return;
        }
        std::string msg = ":" + client->getNickname() + "!" + client->getUsername() +
                          "@localhost PRIVMSG " + target + " :" + message;
        sendResponse(targetClient, msg);
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
    std::string channelName = params[0];
    if (channelName.empty() || channelName[0] != '#')
    {
        sendResponse(client, ":ft_irc 403 " + channelName + " :Invalid channel name (must begin with '#')");
        return;
    }
    
    // Récupérer le client ciblé par son surnom, puis son ID
    int targetFd = server->getFdByNickname(params[1]);
    if (targetFd == -1)
    {
        sendResponse(client, "401 " + params[1] + " :No such nick/channel");
        return;
    }
    Client* targetClient = server->getClientByFd(targetFd);
    if (!targetClient)
    {
        sendResponse(client, "402 " + params[1] + " :No such nick/channel");
        return;
    }
    if (targetClient->getId() == client->getId())
    {
        sendResponse(client, "502 :Cannot kick yourself");
        return;
    }
    
    Channel* channel = server->getChannelByName(channelName);
    if (!channel)
    {
        sendResponse(client, ":ft_irc 403 " + channelName + " :No such channel");
        return;
    }
    
    if (!channel->isClientInChannel(targetClient->getId()))
    {
        sendResponse(client, "441 " + params[1] + " " + channelName + " :They aren't on that channel");
        return;
    }
    if (!channel->isOperator(client->getId()))
    {
        sendResponse(client, "482 " + channelName + " :You're not a channel operator");
        return;
    }
    
    // Notifier le client ciblé
    sendResponse(targetClient, ":localhost NOTICE KICK :You have been kicked from " + channelName);
    
    std::string kickMsg = ":" + client->getNickname() + " KICK " + channelName + " " + params[1] + " :Kicked by " + client->getNickname();
    server->broadcastToChannel(channelName, kickMsg, -1);
    
    // Retirer le client du channel via son ID
    server->kickClient(client->getFd(), channelName, targetFd);
}

// Handler pour la commande INVITE
void handleInvite(Server* server, Client* client, const std::vector<std::string>& params)
{
    if (params.size() < 2)
    {
        sendResponse(client, ERR_NEEDMOREPARAMS(client->getNickname(), "INVITE"));
        return;
    }
    
    std::string channelName = params[0];
    if (channelName.empty() || channelName[0] != '#')
    {
        sendResponse(client, ":ft_irc 403 " + channelName + " :Invalid channel name (must begin with '#')");
        return;
    }
    
    int targetFd = server->getFdByNickname(params[1]);
    if (targetFd == -1)
    {
        sendResponse(client, "401 " + params[1] + " :No such nick/channel");
        return;
    }
    
    if (targetFd == client->getFd())
    {
        sendResponse(client, "502 :Cannot invite yourself");
        return;
    }
    
    Channel* channel = server->getChannelByName(channelName);
    if (!channel)
    {
        sendResponse(client, ERR_NOSUCHCHANNEL(client->getNickname(), channelName));
        return;
    }
    
    Client* invitedClient = server->getClientByFd(targetFd);
    if (!invitedClient)
    {
        sendResponse(client, "401 " + params[1] + " :No such nick/channel");
        return;
    }
    
    if (channel->isClientInChannel(invitedClient->getId()))
    {
        sendResponse(client, ERR_USERONCHANNEL(client->getUsername(), client->getNickname(), channelName));
        return;
    }
    if (channel->isClientInvited(invitedClient->getId()))
    {
        sendResponse(client, "443 " + params[1] + " " + channelName + " :is already invited");
        return;
    }
    
    std::cout << "DEBUG: Tentative d'inviter le client ID " << invitedClient->getId() 
              << " au channel " << channelName << std::endl;
    
    // Envoyer la commande INVITE formatée au client invité
    sendResponse(invitedClient, ":" + client->getNickname() + "!" + client->getUsername() +
                    "@localhost INVITE " + invitedClient->getNickname() + " :" + channelName);
    std::cout << "DEBUG: Invitation envoyée au client ID " << invitedClient->getId() 
              << " pour le channel " << channelName << std::endl;
    
    // Ajouter l'ID invité dans le channel
    if (!channel->isClientInvited(invitedClient->getId()))
        channel->addInvitedClient(invitedClient->getId());
    
    // Diffuser éventuellement le JOIN du client invité s'il est déjà présent
    if (channel->isClientInChannel(invitedClient->getId()))
    {
        std::string joinMsg = ":" + invitedClient->getNickname() + "!" + invitedClient->getUsername() +
                              "@localhost JOIN " + channelName;
        server->broadcastToChannel(channelName, joinMsg, -1);
    }
    std::cout << "DEBUG: Le client avec ID " << invitedClient->getId() 
              << " a été invité avec succès à " << channelName << std::endl;
    
    sendResponse(client, ":ft_irc 341 " + channelName + " " + invitedClient->getNickname() +
                    " :Successfuly added to invite list");
}

// Handler pour la commande PART
void handlePart(Server* server, Client* client, const std::vector<std::string>& params)
{
    if (params.empty())
    {
        sendResponse(client, "461 PART :Not enough parameters");
        return;
    }
    std::string channelName = params[0];
    if (channelName.empty() || channelName[0] != '#')
    {
        sendResponse(client, ":ft_irc 403 " + channelName + " :Invalid channel name (must begin with '#')");
        return;
    }
    
    Channel* channel = server->getChannelByName(channelName);
    if (!channel)
    {
        sendResponse(client, ":ft_irc 403 " + channelName + " :No such channel");
        return;
    }
    if (!channel->isClientInChannel(client->getId()))
    {
        sendResponse(client, ":ft_irc 442 " + channelName + " :You're not on that channel");
        return;
    }
    channel->removeClient(client->getId());
    sendResponse(client, ":ft_irc PART " + channelName + " :You have left the channel");
    std::string partMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost PART " + channelName;
    server->broadcastToChannel(channelName, partMsg, client->getFd());
}

// Handler pour la commande TOPIC
void handleTopic(Server* server, Client* client, const std::vector<std::string>& params)
{
    if (params.size() < 1)
    {
        sendResponse(client, "Invalid arguments\nUsage for TOPIC: TOPIC <channel_name> (new topic)");
        return;
    }

    std::string channelName = params[0];
    if (channelName.empty() || channelName[0] != '#')
    {
        sendResponse(client, ":ft_irc 403 " + channelName + " :Invalid channel name (must begin with '#')");
        return;
    }

    Channel* channel = server->getChannelByName(channelName);
    if (!channel)
    {
        sendResponse(client, ":ft_irc 403 " + channelName + " :No such channel");
        return;
    }

    if (params.size() == 1)
    {
        std::string current_topic = channel->getTopic();
        if (current_topic.empty())
            sendResponse(client, "There's no topic for this channel yet.");
        else
            sendResponse(client, "Current topic of the channel: " + current_topic);
        return;
    }

    std::string new_topic;
    for (size_t i = 1; i < params.size(); i++)
    {
        if (i > 1)
            new_topic += " ";
        new_topic += params[i];
    }
    channel->setTopic(new_topic);

    std::string topicMsg = ":" + client->getNickname() + "!" + client->getUsername() +
                           "@localhost TOPIC " + channelName + " :" + new_topic;
    server->broadcastToChannel(channelName, topicMsg, -1);
}

// Handler pour la commande MODE (dummy)
void handleMode(Server* server, Client* client, const std::vector<std::string>& params)
{
    if (params.size() < 2)
    {
        sendResponse(client, "Invalid arguments\nUsage for MODE: MODE #<channel_name> <mode parameters>");
        return;
    }
    std::string channelName = params[0];
    std::string modeParametre = params[1];

    if (!channelName.empty())
    {
        if (client->getNickname() == channelName)
            return;
        else if (channelName[0] != '#')
        {
            sendResponse(client, ":ft_irc 403 " + channelName + " :Invalid channel name (must begin with '#')");
            return;
        }
    }
    if (modeParametre.empty())
    {
        sendResponse(client, ":ft_irc 403 " + modeParametre + " :Invalid parameters");
        return;
    }
    Channel* channel = server->getChannelByName(channelName);
    // Utiliser l'ID unique pour vérifier l'opérateur
    if (!channel)
    {
        sendResponse(client, ":ft_irc 403 " + channelName + " :No such channel");
        return;
    }
    if (!channel->isOperator(client->getId()))
    {
        sendResponse(client, ":ft_irc 403 " + channelName + " :Client: " + client->getNickname() + " not allow to MODE");
        return;
    }
    size_t paramsIdx = 2;
    char sign = '=';
    for (size_t i = 0; i < modeParametre.size(); i++)
    {
        if (modeParametre[i] == '-' || modeParametre[i] == '+')
        {
            sign = modeParametre[i];
            continue;
        }
        switch (modeParametre[i])
        {
            case 'i': // invite-only
                channel->setInviteOnly(sign == '+');
                break;
            case 't': // topic restricted
                channel->setTopicRestricted(sign == '+');
                break;
            case 'k': // clé (password)
                if (sign == '+')
                {
                    if (paramsIdx < params.size())
                        channel->setKey(params[paramsIdx++]);
                    else
                    {
                        sendResponse(client, ":ft_irc 461 " + channelName + " :Key parameter missing for +k");
                        return;
                    }
                }
                else
                {
                    channel->setKey("");
                }
                break;
            case 'o': // opérateur
                if (paramsIdx < params.size())
                {
                    if (sign == '+')
                    {
                        int targetFd = server->getFdByNickname(params[paramsIdx++]);
                        Client* target = server->getClientByFd(targetFd);
                        if (target)
                            channel->addOperator(target->getId());
                    }
                    else
                    {
                        int aimFd = server->getFdByNickname(params[paramsIdx++]);
                        Client* target = server->getClientByFd(aimFd);
                        if (target)
                        {
                            if (!channel->isOperator(target->getId()))
                                channel->removeOperator(target->getId());
                            else
                            {
                                sendResponse(client, ":ft_irc 461 " + channelName + " :Operator cant be remove");
                                return;
                            }
                        }
                    }
                }
                else
                {
                    sendResponse(client, ":ft_irc 461 " + channelName + " :Operator parameter missing for mode o");
                    return;
                }
                break;
            case 'l': // limite d'utilisateurs
                if (sign == '+')
                {
                    if (paramsIdx < params.size())
                    {
                        int limit = std::atoi(params[paramsIdx++].c_str());
                        channel->setUserLimit(limit);
                    }
                    else
                    {
                        sendResponse(client, ":ft_irc 461 " + channelName + " :User limit parameter missing for +l");
                        return;
                    }
                }
                else
                {
                    channel->setUserLimit(0);
                }
                break;
            default:
                sendResponse(client, ERR_UMODEUNKNOWNFLAG(client->getNickname()));
                break;
        }
    }
}

void dispatchCommand(Server* server, Client* client, const std::vector<std::string>& tokens)
{
    if (tokens.empty())
        return;
        
    std::string cmd = tokens[0];
    for (size_t i = 0; i < cmd.size(); i++)
        cmd[i] = std::toupper(cmd[i]);
    std::vector<std::string> params(tokens.begin() + 1, tokens.end());
                      
    switch (client->getState())
    {
        case WAITING_FOR_PASS:
            if (cmd == "PASS")
            {
                if (handlePass(client, params) == true)
                    client->setState(WAITING_FOR_NICK);
            }
            else
                sendResponse(client, "451 :You have not registered - PASS required first");
            return;
            
        case WAITING_FOR_NICK:
            if (cmd == "NICK")
            {
                if (handleNick(server, client, params) == true)
                    client->setState(WAITING_FOR_USER);
            }
            else
                sendResponse(client, "451 :You have not registered - NICK required next");
            return;
            
        case WAITING_FOR_USER:
            if (cmd == "USER")
            {
                if (handleUser(client, params) == true){
                    client->setState(REGISTERED);
                    sendResponse(client, RPL_WELCOME(user_id(client->getNickname(), client->getUsername()), client->getNickname()));}
            }
            else
                sendResponse(client, "451 :You have not registered - USER required next");
            return;
            
        case REGISTERED:
            if (cmd == "JOIN")
                handleJoin(server, client, params);
            else if (cmd == "PRIVMSG")
                handlePrivmsg(server, client, params);
            else if (cmd == "KICK")
                handleKick(server, client, params);
            else if (cmd == "INVITE")
                handleInvite(server, client, params);
            else if (cmd == "PART")
                handlePart(server, client, params);
            else if (cmd == "TOPIC")
                handleTopic(server, client, params);
            else if (cmd == "MODE")
                handleMode(server, client, params);
            else if (cmd == "PING")
                handlePing(client, params);
            else
                sendResponse(client, "421 " + tokens[0] + " :Unknown command");
            return;
    }
}

void parseAndDispatch(Server* server, Client* client, const std::string &message)
{
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
                sendResponse(client, "421 :Empty command received");
            else
            {
                std::cout << "Commande détectée: " << tokens[0] << "\n";
                dispatchCommand(server, client, tokens);
            }
        }
        start = end + 2;
        end = message.find("\r\n", start);
    }
    
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
