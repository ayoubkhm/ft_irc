#ifndef CHANNEL_HPP
#define CHANNEL_HPP

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
    std::set<int> _clients;
    std::set<int> _operators;
    std::string _mode;
    size_t _maxClients;
    /*************************************/
    bool _inviteOnly;
    bool _topicRestricted;
    std::string _key;
    int _userLimit;

public:
    Channel(const std::string& channelName, size_t maxClients = 50, const std::string& mode = "");
    void addClient(int fd);
    void removeClient(int fd);
    bool isClientInChannel(int fd) const;
    bool isOperator(int fd) const;
    void addOperator(int fd);
    void removeOperator(int fd);
    void setTopic(const std::string& newTopic);
    const std::string& getTopic() const;
    const std::string& getMode() const;
    size_t getMaxClients() const;
    void printClients() const;
    size_t getClientCount() const;
    // Nouvelle méthode pour récupérer les fds des clients
    const std::set<int>& getClientFds() const;
    // Getters et setters pour les modes
    void setInviteOnly(bool inviteOnly);
    bool getInviteOnly() const;
    
    void setTopicRestricted(bool topicRestricted);
    bool getTopicRestricted() const;
    
    void setKey(const std::string& key);
    const std::string& getKey() const;
    
    void setUserLimit(int limit);
    int getUserLimit() const;
};

#endif
