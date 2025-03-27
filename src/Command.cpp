// Command.cpp

#include "Command.hpp"
#include "IRCUtils.hpp" // Déclare sendResponse(...)
#include "Server.hpp"
#include "RPL.hpp"


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
        // pareil que PASS
        //sendResponse(client, "409 :No origin specified");
        return;
    }
    sendResponse(client, RPL_PONG(user_id(client->getNickname(), client->getUsername()), params[0]));
}

// Handler pour la commande PASS
bool handlePass(Client* client, const std::vector<std::string>& params)
{
    if (params.empty())
    {
        sendResponse(client, ERR_NEEDMOREPARAMS(client->getNickname(), "PASS"));
        return false;
    }
    client->authenticate(params[0]);
    if (client->isAuthenticated()){
        // Vous pouvez définir une macro spécifique pour cette notification si besoin.
        // il nexsite pas de RPL specifique, dapres GPT, je suis pour lenlever au risque de se faire niquer parce que on ne suit pas le protocol RPL et IRC
        //sendResponse(client, "NOTICE :Mot de passe accepté\r\n");
        return true;
    }
    else {
        sendResponse(client, ERR_PASSWDMISMATCH(client->getNickname()));
        return false;
    }
}


// Handler pour la commande NICK
bool handleNick(Server* server, Client* client, const std::vector<std::string>& params)
{
    if (params.empty())
    {
        sendResponse(client, ERR_NEEDMOREPARAMS(client->getNickname(), "NICK"));
        return false;
    }
    std::string nickName = params[0];
    if (server->checkDuplicateClient(nickName))
    {
        sendResponse(client, ERR_NICKNAMEINUSE(client->getNickname(), nickName));
        return false;
    }
    client->setNickname(const_cast<std::string&>(params[0]));
    return true;
}


// Handler pour la commande USER
bool handleUser(Client* client, const std::vector<std::string>& params)
{
    if (params.size() < 1)
    {
        sendResponse(client, ERR_NEEDMOREPARAMS(client->getNickname(), "USER"));
        return false;
    }
    client->setUsername(const_cast<std::string&>(params[0]));
    return true;
}


// Handler pour la commande JOIN
void handleJoin(Server* server, Client* client, const std::vector<std::string>& params)
{
    // Vérification du nombre de paramètres
    if (params.empty() || (params[0].size() == 1 && params[0][0] == '#'))
    {
        sendResponse(client, ERR_NEEDMOREPARAMS(client->getNickname(), "JOIN"));
        return;
    }
    std::string channelName = params[0];
    // Vérification du nom de channel
    
    if (channelName.empty() || channelName[0] != '#')
    {
        sendResponse(client, ERR_NOSUCHCHANNEL(client->getNickname(), "#" + channelName));
        return;
    }
    
    Channel* channel = server->getChannelByName(channelName);
    if (channel)
    {
        // Utilisation de l'ID unique pour les vérifications
        if (channel->isClientInChannel(client->getId()))
        {
            sendResponse(client, ERR_ALREADYJOINED(client->getNickname(), channelName));
            return;
        }
        if (channel->getInviteOnly() && !channel->isClientInvited(client->getId()))
        {
            sendResponse(client, ERR_INVITEONLY(client->getNickname(), channelName));
            return;
        }
        if (!channel->getKey().empty())
        {
            if (params.size() < 2)
            {
                sendResponse(client, ERR_BADCHANNELKEY(client->getNickname(), channelName));
                return;
            }
            std::string mdp = params[1];
            if (channel->getKey() != mdp)
            {
                sendResponse(client, ERR_BADCHANNELKEY(client->getNickname(), channelName));
                return;
            }
        }
    }
    server->joinChannel(client->getFd(), channelName);
    channel = server->getChannelByName(channelName);
    
    if (channel && channel->isClientInChannel(client->getId()))
    {
        channel->removeInvitedClient(client->getId());
        std::string joinMsg = RPL_JOIN(user_id(client->getNickname(), client->getUsername()), channelName);
        server->broadcastToChannel(channelName, joinMsg, -1);
        // doublon
        //sendResponse(client, RPL_JOIN(user_id(client->getNickname(), client->getUsername()), channelName));
    }
    else
    {
        sendResponse(client, ERR_NOSUCHCHANNEL(client->getNickname(), channelName));
    }
}


void handlePrivmsg(Server* server, Client* client, const std::vector<std::string>& params)
{
    if (params.size() < 1)
    {
        sendResponse(client, ERR_NORECIPIENT(client->getNickname()));
        return;
    }
    if (params.size() == 1)
    {
        sendResponse(client, ERR_NOTEXTTOSEND(client->getNickname()));
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
    
    // Si le message est destiné à un channel
    if (!target.empty() && target[0] == '#')
    {
        Channel* channel = server->getChannelByName(target);
        if (channel == NULL || !channel->isClientInChannel(client->getId()))
        {
            sendResponse(client, ERR_NOSUCHNICK(client->getNickname(), target));
            return;
        }
        std::string msg = RPL_PRIVMSG(client->getNickname(), client->getUsername(), target, message);
        server->broadcastToChannel(target, msg, client->getFd());
        //sendResponse(client, "NOTICE PRIVMSG :Message delivered to " + target + "\r\n");
    }
    // Message privé
    else if (!target.empty() && target[0] != '#' && target != client->getNickname())
    {
        int targetFd = server->getFdByNickname(target);
        if (targetFd == -1)
        {
            sendResponse(client, ERR_NOSUCHNICK(client->getNickname(), target));
            return;
        }
        Client* targetClient = server->getClientByFd(targetFd);
        if (!targetClient)
        {
            sendResponse(client, ERR_NOSUCHNICK(client->getNickname(), target));
            return;
        }
        std::string msg = RPL_PRIVMSG(client->getNickname(), client->getUsername(), target, message);
        sendResponse(targetClient, msg);
        //sendResponse(client, "NOTICE PRIVMSG :Message delivered to " + target + "\r\n");
    }
    else
    {
        sendResponse(client, ERR_NOSUCHNICK(client->getNickname(), target));
    }
}


void handleKick(Server* server, Client* client, const std::vector<std::string>& params)
{
    if (params.size() < 2)
    {
        sendResponse(client, ERR_NEEDMOREPARAMS(client->getNickname(), "KICK"));
        return;
    }
    
    std::string channelName = params[0];
    if (channelName.empty() || channelName[0] != '#')
    {
        sendResponse(client, ERR_NOSUCHCHANNEL(client->getNickname(), channelName));
        return;
    }
    
    // Récupérer le client ciblé par son surnom, puis son ID
    int targetFd = server->getFdByNickname(params[1]);
    if (targetFd == -1)
    {
        sendResponse(client, ERR_NOSUCHNICK(client->getNickname(), params[1]));
        return;
    }
    
    Client* targetClient = server->getClientByFd(targetFd);
    if (!targetClient)
    {
        sendResponse(client, ERR_NOSUCHNICK(client->getNickname(), params[1]));
        return;
    }
    
    if (targetClient->getId() == client->getId())
    {
        sendResponse(client, ERR_CANNOTKICKSELF(client->getNickname()));
        return;
    }
    
    Channel* channel = server->getChannelByName(channelName);
    if (!channel)
    {
        sendResponse(client, ERR_NOSUCHCHANNEL(client->getNickname(), channelName));
        return;
    }
    
    if (!channel->isClientInChannel(targetClient->getId()))
    {
        sendResponse(client, ERR_USERNOTINCHANNEL(client->getNickname(), params[1], channelName));
        return;
    }
    
    if (!channel->isOperator(client->getId()))
    {
        sendResponse(client, ERR_CHANOPRIVSNEEDED(client->getNickname(), channelName));
        return;
    }
    
    // Notifier le client ciblé qu'il va être kické
    //sendResponse(targetClient, ":localhost NOTICE KICK :You have been kicked from " + channelName + "\r\n");
    
    // Construire le message KICK en utilisant la macro RPL_KICK
    std::string kickMsg = RPL_KICK(channelName, params[1], "Kicked by " + client->getNickname());
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
        sendResponse(client, ERR_NOSUCHCHANNEL(client->getNickname(), channelName));
        return;
    }
    
    int targetFd = server->getFdByNickname(params[1]);
    if (targetFd == -1)
    {
        sendResponse(client, ERR_NOSUCHNICK(client->getNickname(), params[1]));
        return;
    }
    
    if (targetFd == client->getFd())
    {
        sendResponse(client, ERR_INVITEYOURSELF(client->getNickname()));
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
        sendResponse(client, ERR_NOSUCHNICK(client->getNickname(), params[1]));
        return;
    }
    
    if (channel->isClientInChannel(invitedClient->getId()))
    {
        sendResponse(client, ERR_USERONCHANNEL(client->getUsername(), client->getNickname(), channelName));
        return;
    }
    if (channel->isClientInvited(invitedClient->getId()))
    {
        sendResponse(client, ERR_ALREADYINVITED(client->getNickname(), channelName));
        return;
    }
    
    std::cout << "DEBUG: Tentative d'inviter le client ID " << invitedClient->getId() 
              << " au channel " << channelName << std::endl;
    
    // Envoyer la commande INVITE formatée au client invité avec la macro RPL_INVITE
    sendResponse(invitedClient, RPL_INVITE(user_id(client->getNickname(), client->getUsername()),
                                           invitedClient->getNickname(), channelName));
    std::cout << "DEBUG: Invitation envoyée au client ID " << invitedClient->getId() 
              << " pour le channel " << channelName << std::endl;
    
    // Ajouter l'ID invité dans le channel
    if (!channel->isClientInvited(invitedClient->getId()))
        channel->addInvitedClient(invitedClient->getId());
    
    // Diffuser éventuellement le JOIN si le client invité est déjà présent dans le channel
    if (channel->isClientInChannel(invitedClient->getId()))
    {
        std::string joinMsg = ":" + invitedClient->getNickname() + "!" + invitedClient->getUsername() +
                              "@localhost JOIN " + channelName + "\r\n";
        server->broadcastToChannel(channelName, joinMsg, -1);
    }
    std::cout << "DEBUG: Le client avec ID " << invitedClient->getId() 
              << " a été invité avec succès à " << channelName << std::endl;
    
    // Notifier l'émetteur que l'invitation a été effectuée via la macro RPL_INVITING
    sendResponse(client, RPL_INVITING(client->getNickname(), invitedClient->getNickname(), channelName));
}

void handlePart(Server* server, Client* client, const std::vector<std::string>& params)
{
    if (params.empty())
    {
        sendResponse(client, ERR_NEEDMOREPARAMS(client->getNickname(), "PART"));
        return;
    }
    std::string channelName = params[0];
    if (channelName.empty() || channelName[0] != '#')
    {
        sendResponse(client, ERR_NOSUCHCHANNEL(client->getNickname(), channelName));
        return;
    }
    
    Channel* channel = server->getChannelByName(channelName);
    if (!channel)
    {
        sendResponse(client, ERR_NOSUCHCHANNEL(client->getNickname(), channelName));
        return;
    }
    if (!channel->isClientInChannel(client->getId()))
    {
        sendResponse(client, ERR_NOTONCHANNEL(client->getNickname(), channelName));
        return;
    }
    channel->removeClient(client->getId());
    sendResponse(client, RPL_PART(user_id(client->getNickname(), client->getUsername()), channelName, "You have left the channel"));
    std::string partMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost PART " + channelName + "\r\n";
    server->broadcastToChannel(channelName, partMsg, client->getFd());
}

// Handler pour la commande TOPIC
void handleTopic(Server* server, Client* client, const std::vector<std::string>& params)
{
    if (params.size() < 1)
    {
        sendResponse(client, "Invalid arguments\nUsage for TOPIC: TOPIC <channel_name> (new topic)\r\n");
        return;
    }

    std::string channelName = params[0];
    if (channelName.empty() || channelName[0] != '#')
    {
        sendResponse(client, ERR_NOSUCHCHANNEL(client->getNickname(), channelName));
        return;
    }

    Channel* channel = server->getChannelByName(channelName);
    if (!channel)
    {
        sendResponse(client, ERR_NOSUCHCHANNEL(client->getNickname(), channelName));
        return;
    }

    // Si aucun nouveau topic n'est fourni, on renvoie le topic actuel
    if (params.size() == 1)
    {
        std::string current_topic = channel->getTopic();
        if (current_topic.empty())
            sendResponse(client, RPL_NOTOPIC(client->getNickname(), channelName));
        else
            sendResponse(client, RPL_TOPIC(client->getNickname(), channelName, current_topic));
        return;
    }

    // Construction du nouveau topic à partir des paramètres
    std::string new_topic;
    for (size_t i = 1; i < params.size(); i++)
    {
        if (i > 1)
            new_topic += " ";
        new_topic += params[i];
    }
    channel->setTopic(new_topic);

    // Diffuser le message TOPIC au channel
    std::string topicMsg = ":" + client->getNickname() + "!" + client->getUsername() +
                           "@localhost TOPIC " + channelName + " :" + new_topic + "\r\n";
    server->broadcastToChannel(channelName, topicMsg, -1);
}

// Handler pour la commande MODE (dummy)
void handleMode(Server* server, Client* client, const std::vector<std::string>& params)
{
    // Vérification du nombre d'arguments
    if (params.size() < 2)
    {
        sendResponse(client, ERR_NEEDMOREPARAMS(client->getNickname(), "MODE"));
        return;
    }
    
    std::string channelName = params[0];
    std::string modeParametre = params[1];

    // Vérification du nom de channel
    if (!channelName.empty())
    {
        // Si le channelName n'est pas un channel (commence par '#')
        if (channelName[0] != '#')
        {
            sendResponse(client, ERR_NOSUCHCHANNEL(client->getNickname(), channelName));
            return;
        }
    }
    if (modeParametre.empty())
    {
        sendResponse(client, ERR_NEEDMOREPARAMS(client->getNickname(), "MODE"));
        return;
    }
    
    Channel* channel = server->getChannelByName(channelName);
    if (!channel)
    {
        sendResponse(client, ERR_NOSUCHCHANNEL(client->getNickname(), channelName));
        return;
    }
    // Vérification des privilèges d'opérateur via l'ID unique
    if (!channel->isOperator(client->getId()))
    {
        sendResponse(client, ERR_CHANOPRIVSNEEDED(client->getNickname(), channelName));
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
            case 'i': // Mode invite-only
                channel->setInviteOnly(sign == '+');
                break;
            case 't': // Mode topic restricted
                channel->setTopicRestricted(sign == '+');
                break;
            case 'k': // Mode clé (password)
                if (sign == '+')
                {
                    if (paramsIdx < params.size())
                        channel->setKey(params[paramsIdx++]);
                    else
                    {
                        sendResponse(client, ERR_NEEDMOREPARAMS(client->getNickname(), "MODE +k"));
                        return;
                    }
                }
                else
                {
                    channel->setKey("");
                }
                break;
            case 'o': // Donner ou retirer le statut d'opérateur
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
                            // On vérifie ici que l'opérateur peut être retiré
                            if (!channel->isOperator(target->getId()))
                                channel->removeOperator(target->getId());
                            else
                            {
                                sendResponse(client, ERR_CANNOTREMOVEOP(client->getNickname(), channelName));
                                return;
                            }
                        }
                    }
                }
                else
                {
                    sendResponse(client, ERR_NEEDMOREPARAMS(client->getNickname(), "MODE o"));
                    return;
                }
                break;
            case 'l': // Limite du nombre d'utilisateurs
                if (sign == '+')
                {
                    if (paramsIdx < params.size())
                    {
                        int limit = std::atoi(params[paramsIdx++].c_str());
                        channel->setUserLimit(limit);
                    }
                    else
                    {
                        sendResponse(client, ERR_NEEDMOREPARAMS(client->getNickname(), "MODE +l"));
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
                sendResponse(client, ERR_NOTREGISTERED(client->getNickname()));
            return;
            
        case WAITING_FOR_NICK:
            if (cmd == "NICK")
            {
                if (handleNick(server, client, params) == true)
                    client->setState(WAITING_FOR_USER);
            }
            else
                sendResponse(client, ERR_NOTREGISTERED(client->getNickname()));
            return;
            
        case WAITING_FOR_USER:
            if (cmd == "USER")
            {
                if (handleUser(client, params) == true)
                {
                    client->setState(REGISTERED);
                    sendResponse(client, RPL_WELCOME(user_id(client->getNickname(), client->getUsername()), client->getNickname()));
                }
            }
            else
                sendResponse(client, ERR_NOTREGISTERED(client->getNickname()));
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
                sendResponse(client, ERR_UNKNOWNCOMMAND(client->getNickname(), tokens[0]));
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
                sendResponse(client, ERR_UNKNOWNCOMMAND(client->getNickname(), tokens[0]));
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
