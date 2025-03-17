#include "Command.hpp"
#include "IRCUtils.hpp" // Déclare sendResponse(...)
#include "Server.hpp"
#include <iostream>
#include <vector>
#include <cctype>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <cstdlib>

// Fonctions d'assistance

// Fonction qui check si la string passée en paramètre est un nom de channel, si oui, le renvoie
Channel* Server::getChannelByName(const std::string& channelName)
{
    std::map<std::string, Channel>::iterator it = _Channels.find(channelName);
    if (it != _Channels.end())
    {
        return &it->second;
    }
    return NULL;
}

// Handler pour la commande CAP
void handleCap(Client* client, const std::vector<std::string>& params)
{
    (void)params; // Paramètre inutilisé
    // Renvoie une liste de capacités non vide pour satisfaire irssi
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
void handleNick(Server* server ,Client* client, const std::vector<std::string>& params)
{
    if (params.empty())
    {
        sendResponse(client, "461 NICK :Not enough parameters");
        return;
    }
    std::string nickName = params[0];
    if (server->checkDuplicateClient(nickName) == true)
    {
        sendResponse(client, "461 NICK : " + nickName + " Duplicate NICK");
        return;
    }
    client->setNickname(const_cast<std::string&>(params[0]));
    // Ne pas envoyer d'écho welcome ici pour éviter les doublons.
}

// Handler pour la commande USER
void handleUser(Client* client, const std::vector<std::string>& params)
{
    if (params.size() < 1)
    {
        sendResponse(client, "461 USER :Not enough parameters");
        return;
    }
    client->setUsername(const_cast<std::string&>(params[0]));
    // Ne pas envoyer d'écho welcome ici ; ce sera géré dans dispatchCommand.
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
    // Vérifier que le nom du channel commence par '#'
    if (channelName.empty() || channelName[0] != '#')
    {
        sendResponse(client, ":ft_irc 403 " + channelName + " :Invalid channel name (must begin with '#')");
        return;
    }
    
    // Ajouter le client au channel
    server->joinChannel(client->getFd(), channelName);
    
    // Récupérer le channel
    Channel* channel = server->getChannelByName(channelName);
    if (channel && channel->isClientInChannel(client->getFd()))
    {
        // Construire le message de JOIN avec préfixe complet
        std::string joinMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost JOIN " + channelName;
        server->broadcastToChannel(channelName, joinMsg, -1);
        // Envoyer une confirmation au client avec préfixe du serveur
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
    std::string message = "";
    size_t i = 1;
    while (i < params.size())
    {
        if (i > 1)
            message += " ";
        message += params[i];
        i++;
    }
    
    // Si le message est destiné à un channel
    if (!target.empty() && target[0] == '#')
    {
        // Vérifier que le client émetteur est bien membre du channel
        Channel* channel = server->getChannelByName(target);
        if (channel == NULL || !channel->isClientInChannel(client->getFd()))
        {
            sendResponse(client, "442 " + target + " :You're not on that channel");
            return;
        }
        
        std::string msg = ":" + client->getNickname() + " PRIVMSG " + target + " :" + message;
        server->broadcastToChannel(target, msg, client->getFd());
        sendResponse(client, "NOTICE PRIVMSG :Message delivered to " + target);
    }
    // Si le message est destiné à un utilisateur privé
    else if (!target.empty() && target[0] != '#' && target != client->getNickname())
    {
        int targetClientFd = server->getFdByNickname(target);
        if (targetClientFd == -1)
        {
            sendResponse(client, "401 " + target + " :No such nick/channel");
            return;
        }
        Client* targetclient = server->getClientByFd(targetClientFd);
        if (!targetclient)
        {
            sendResponse(client, "402 " + target + " :No such nick/channel");
            return;
        }
        // Construire le message privé complet avec préfixe
        std::string msg = ":" + client->getNickname() + "!" + client->getUsername() +
                          "@localhost PRIVMSG " + target + " :" + message;
        sendResponse(targetclient, msg);
        sendResponse(client, "NOTICE PRIVMSG :Message delivered to " + target);
    }
    else
    {
        sendResponse(client, "401 " + target + " :No such nick/channel");
    }
}


// Handler pour la commande KICK
// Handler pour la commande KICK
void handleKick(Server* server, Client* client, const std::vector<std::string>& params)
{
    if (params.size() < 2)
    {
        sendResponse(client, "461 KICK :Not enough parameters");
        return;
    }
    std::string channelName = params[0];
    // Vérifier que le nom du channel commence par '#'
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
        sendResponse(client, "502 :Cannot kick yourself");
        return;
    }
    
    Channel* channel = server->getChannelByName(channelName);
    if (!channel)
    {
        sendResponse(client, ":ft_irc 403 " + channelName + " :No such channel");
        return;
    }
    
    if (!channel->isClientInChannel(targetFd))
    {
        sendResponse(client, "441 " + params[1] + " " + channelName + " :They aren't on that channel");
        return;
    }
    if (!channel->isOperator(client->getFd()))
    {
        sendResponse(client, "482 " + channelName + " :You're not a channel operator");
        return;
    }
    
    // Notifier le client ciblé qu'il va être kické
    Client* kickedClient = server->getClientByFd(targetFd);
    if (kickedClient)
    {
        // Ici, on envoie une notification avec le préfixe du serveur
        sendResponse(kickedClient, ":localhost NOTICE KICK :You have been kicked from " + channelName);
    }
    
    // Diffuser un message KICK à tous les membres du channel
    // Exemple : ":operator KICK #channel target :Kicked by operator"
    std::string kickMsg = ":" + client->getNickname() + " KICK " + channelName + " " + params[1] + " :Kicked by " + client->getNickname();
    server->broadcastToChannel(channelName, kickMsg, -1);
    
    // Retirer le client du channel
    server->kickClient(client->getFd(), channelName, targetFd);
}


// Handler pour la commande INVITE
void handleInvite(Server* server, Client* client, const std::vector<std::string>& params)
{
    if (params.size() < 2)
    {
        sendResponse(client, "461 INVITE :Not enough parameters");
        return;
    }
    
    std::string channelName = params[0];
    // Vérifier que le nom du channel commence par '#'
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
        sendResponse(client, ":ft_irc 403 " + channelName + " :No such channel");
        return;
    }
    
    if (channel->isClientInChannel(targetFd))
    {
        sendResponse(client, "443 " + params[1] + " " + channelName + " :is already on that channel");
        return;
    }
    
    std::cout << "DEBUG: Tentative d'inviter le client fd " << targetFd 
              << " au channel " << channelName << std::endl;
    
    Client* invitedClient = server->getClientByFd(targetFd);
    if (!invitedClient)
    {
        sendResponse(client, "401 " + params[1] + " :No such nick/channel");
        return;
    }
    
    // Envoyer au client invité une commande INVITE formatée correctement :
    // Par exemple : ":youb!<username>@localhost INVITE debian :#test"
    sendResponse(invitedClient, ":" + client->getNickname() + "!" + client->getUsername() +
                    "@localhost INVITE " + invitedClient->getNickname() + " :" + channelName);
    std::cout << "DEBUG: Invitation envoyée au client fd " << targetFd 
              << " pour le channel " << channelName << std::endl;
    
    // Ajouter le client invité dans le channel
    server->joinChannel(targetFd, channelName);
    
    // Diffuser à tous les membres du channel le message JOIN du client invité
    Channel* ch = server->getChannelByName(channelName);
    if (ch && ch->isClientInChannel(targetFd))
    {
        std::string joinMsg = ":" + invitedClient->getNickname() + "!" + invitedClient->getUsername() +
                              "@localhost JOIN " + channelName;
        server->broadcastToChannel(channelName, joinMsg, -1);
    }
    std::cout << "DEBUG: Le client avec fd " << targetFd 
              << " a rejoint le channel " << channelName << " suite à l'invitation." << std::endl;
    
    // Notifier l'émetteur avec un message numérique confirmant l'invitation
    sendResponse(client, ":ft_irc 341 " + channelName + " " + invitedClient->getNickname() +
                    " :Invite successful and user added");
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
    // Vérifier que le nom du channel commence par '#'
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
    if (!channel->isClientInChannel(client->getFd()))
    {
        sendResponse(client, ":ft_irc 442 " + channelName + " :You're not on that channel");
        return;
    }
    channel->removeClient(client->getFd());
    sendResponse(client, ":ft_irc PART " + channelName + " :You have left the channel");
    std::string partMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost PART " + channelName;
    server->broadcastToChannel(channelName, partMsg, client->getFd());
}

// Handler pour la commande TOPIC
void handleTopic(Server* server, Client* client, const std::vector<std::string>& params)
{
    if (params.size() < 1) {
        sendResponse(client, "Invalid arguments\nUsage for TOPIC: TOPIC <channel_name> (new topic)");
        return;
    }

    std::string channelName = params[0];
    // Vérifier que le nom du channel commence par '#'
    if (channelName.empty() || channelName[0] != '#')
    {
        sendResponse(client, ":ft_irc 403 " + channelName + " :Invalid channel name (must begin with '#')");
        return;
    }

    Channel* channel = server->getChannelByName(channelName);
    if (!channel) {
        sendResponse(client, ":ft_irc 403 " + channelName + " :No such channel");
        return;
    }

    if (params.size() == 1) {
        // Afficher le topic actuel
        std::string current_topic = channel->getTopic();
        if (current_topic.empty()) {
            sendResponse(client, "There's no topic for this channel yet.");
        } else {
            sendResponse(client, "Current topic of the channel: " + current_topic);
        }
        return;
    }

    // Mettre à jour le topic avec le nouveau contenu
    std::string new_topic = params[1];
    channel->setTopic(new_topic);

    // Construire le message TOPIC au format IRC
    // Par exemple: ":youb!jfoijf@localhost TOPIC #test :New Topic"
    std::string topicMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost TOPIC " + channelName + " :" + new_topic;
    
    // Diffuser ce message à tous les membres du channel
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
    if (channelName.empty() || channelName[0] != '#') {
        sendResponse(client, ":ft_irc 403 " + channelName + " :Invalid channel name (must begin with '#')");
        return;
    }
    if (modeParametre.empty()) {
        sendResponse(client, ":ft_irc 403 " + modeParametre + " :Invalid parameters");
        return;
    }
    Channel* channel = server->getChannelByName(channelName);
    if (!channel) {
        sendResponse(client, ":ft_irc 403 " + channelName + " :No such channel");
        return;
    }
    size_t paramsIdx = 2;
    char sign = '=';
    for (int i = 0; i < modeParametre.size(); i++)
    {
        if (modeParametre[i] == '-' || modeParametre[i] == '+') {
            sign = modeParametre[i];
            continue;
        }
            switch (modeParametre[i]) {
                case 'i': // Mode invite-only
                    if (sign == '+') {
                        // Activer le mode invite-only pour le channel
                        channel->setInviteOnly(true);
                    } else {
                        // Désactiver le mode invite-only
                        channel->setInviteOnly(false);
                    }
                    break;
                case 't': // Mode topic restricted
                    if (sign == '+') {
                        // Restreindre la modification du topic aux opérateurs
                        channel->setTopicRestricted(true);
                    } else {
                        // Permettre à tous de modifier le topic
                        channel->setTopicRestricted(false);
                    }
                    break;
                case 'k': // Mode clé (password)
                    if (sign == '+') {
                        // Pour activer le mode +k, il faut récupérer en supplément la clé
                        if (paramsIdx < params.size())
                            channel->setKey(params[paramsIdx++]);
                        else {
                            sendResponse(client, ":ft_irc 461 " + channelName + " :Key parameter missing for +k");
                            return;
                        }
                    } else {
                        // Désactiver la clé du channel
                        channel->setKey("");
                    }
                    break;
                case 'o': // Donner ou retirer les privilèges d'opérateur
                    if (paramsIdx < params.size()) {
                        if (sign == '+')
                        {
                            int targetFd = server->getFdByNickname(params[paramsIdx++]);
                            channel->addOperator(targetFd);
                        }
                        else
                        {
                            int aimFd = server->getFdByNickname(params[paramsIdx++]);
                            channel->removeOperator(aimFd);
                        }
                    } else {
                        sendResponse(client, ":ft_irc 461 " + channelName + " :Operator parameter missing for mode o");
                        return;
                    }
                    break;
                case 'l': // Limite du nombre d'utilisateurs
                    if (sign == '+') {
                        if (paramsIdx < params.size()) {
                            int limit = std::atoi(params[paramsIdx++].c_str());
                            channel->setUserLimit(limit);
                        } else {
                            sendResponse(client, ":ft_irc 461 " + channelName + " :User limit parameter missing for +l");
                            return;
                        }
                    } else {
                        channel->setUserLimit(0);
                    }
                    break;
                default:
                    break;
            }
        }
/*     sendResponse(client, "221 :MODE non implémentée"); */
}

void dispatchCommand(Server* server, Client* client, const std::vector<std::string>& tokens)
{
    if (tokens.empty())
        return;
        
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
        handleNick(server, client, params);
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
    else if (cmd == "PART")
    {
        handlePart(server, client, params);
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
            {
                sendResponse(client, "421 :Empty command received");
            }
            else
            {
                std::cout << "Commande détectée: " << tokens[0] << "\n";
                dispatchCommand(server, client, tokens);
            }
        }
        start = end + 2; // Passe "\r\n"
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
