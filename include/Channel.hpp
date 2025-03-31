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
    int _userLimit;
    bool _inviteOnly;
    bool _topicRestricted;
    std::string _name;
    std::string _topic;
    std::string _key;
    std::set<unsigned int> _clients;      // On stocke désormais l'ID unique des clients
    std::set<unsigned int> _operators;
    std::set<unsigned int> _invitedIds;     // Anciennement _invitedFd, on stocke ici les IDs invités
    size_t _maxClients;

public:
    Channel(const std::string& channelName, size_t maxClients = 1024);
    ~Channel();
    Channel(Channel const& copy);
    Channel &operator=(Channel const& copy);

    // Setters
    void setInviteOnly(bool inviteOnly);
    void setKey(const std::string& key);
    void setTopicRestricted(bool topicRestricted);
    void setUserLimit(int limit);
    void setTopic(const std::string& newTopic);

    //Getters
    bool getInviteOnly() const;
    bool getTopicRestricted() const;
    const std::string& getKey() const;
    const std::string& getTopic() const;
    const std::set<unsigned int>& getClientIds() const;      // Renommage de la méthode pour refléter qu'on récupère les IDs
    size_t getMaxClients() const;
    size_t getClientCount() const;

    //Méthodes clients
    bool addClient(unsigned int clientId);
    void removeClient(unsigned int clientId);
    bool isClientInChannel(unsigned int clientId) const;
    void printClients() const;

    //Méthodes operator
    void addOperator(unsigned int clientId);
    void removeOperator(unsigned int clientId);
    bool isOperator(unsigned int clientId) const;

    //Méthodes INVITE
    void addInvitedClient(unsigned int clientId);
    void removeInvitedClient(unsigned int clientId);
    bool isClientInvited(unsigned int clientId) const;
};
