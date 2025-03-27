#pragma once

#include <set>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include "Client.hpp"
#include "IRCUtils.hpp"

class Channel
{
private:
    std::string _name;
    std::string _topic;
    std::set<unsigned int> _clients;      // On stocke désormais l'ID unique des clients
    std::set<unsigned int> _operators;
    std::set<unsigned int> _invitedIds;     // Anciennement _invitedFd, on stocke ici les IDs invités
    size_t _maxClients;
    bool _inviteOnly;
    bool _topicRestricted;
    std::string _key;
    int _userLimit;

public:
    Channel(const std::string& channelName, size_t maxClients = 1024);
    ~Channel();
    Channel(Channel const& copy);
    Channel &operator=(Channel const& copy);

    bool addClient(unsigned int clientId);
    void addInvitedClient(unsigned int clientId);
    void removeInvitedClient(unsigned int clientId);
    void removeClient(unsigned int clientId);
    bool isClientInChannel(unsigned int clientId) const;
    bool isClientInvited(unsigned int clientId) const;
    bool isOperator(unsigned int clientId) const;
    void addOperator(unsigned int clientId);
    void removeOperator(unsigned int clientId);
    void setTopic(const std::string& newTopic);
    const std::string& getTopic() const;
    size_t getMaxClients() const;
    void printClients() const;
    size_t getClientCount() const;
    // Renommage de la méthode pour refléter qu'on récupère les IDs
    const std::set<unsigned int>& getClientIds() const;

    // Getters et setters pour les modes
    void setInviteOnly(bool inviteOnly);
    bool getInviteOnly() const;

    void setTopicRestricted(bool topicRestricted);
    bool getTopicRestricted() const;

    void setKey(const std::string& key);
    const std::string& getKey() const;

    void setUserLimit(int limit);
};
