#include "Channel.hpp"

Channel::Channel(const std::string& channelName, size_t maxClients, const std::string& mode)
    : _name(channelName), _topic(""), _mode(mode), _maxClients(maxClients)
{
}

void Channel::addClient(int fd)
{
    if (_clients.size() >= _maxClients)
    {
        std::cerr << "Le channel " << _name << " est plein !" << std::endl;
        return;
    }
    _clients.insert(fd);
}

void Channel::addInvitedClient(int fd)
{
    _invitedFd.insert(fd);
}

void Channel::removeClient(int fd)
{
    std::set<int>::iterator it = _clients.find(fd);
    if (it != _clients.end())
    {
        _clients.erase(it);
        std::cout << "Client avec fd " << fd << " a été expulsé du channel." << std::endl;
    }
}

bool Channel::isClientInChannel(int fd) const
{
    return _clients.find(fd) != _clients.end();
}

bool Channel::isOperator(int fd) const
{
    return _operators.find(fd) != _operators.end();
}

bool Channel::isClientInvited(int fd) const
{
    return _invitedFd.find(fd) != _invitedFd.end();
}

void Channel::addOperator(int fd)
{
    _operators.insert(fd);
    std::cout << "Client avec fd " << fd << " est maintenant opérateur." << std::endl;
}

void Channel::removeOperator(int fd)
{
    _operators.erase(fd);
    std::cout << "Client avec fd " << fd << " n'est plus opérateur." << std::endl;
}

void Channel::setTopic(const std::string& newTopic)
{
    _topic = newTopic;
    std::cout << "Le topic du channel " << _name << " a été mis à jour : " << _topic << std::endl;
}

const std::string& Channel::getTopic() const
{
    return _topic;
}

const std::string& Channel::getMode() const
{
    static std::string mode;
    mode = "";
    if (_inviteOnly)
        mode += "i";
    if (_topicRestricted)
        mode += "t";
    if (!_key.empty())
        mode += "k";
    if (_userLimit > 0)
        mode += "l";
    return mode;
}

size_t Channel::getMaxClients() const
{
    return _maxClients;
}

void Channel::printClients() const
{
    std::cout << "Clients dans le channel " << _name << " :" << std::endl;
    std::set<int>::const_iterator it = _clients.begin();
    while (it != _clients.end())
    {
        std::cout << "- Client avec fd : " << *it << std::endl;
        ++it;
    }
}

size_t Channel::getClientCount() const
{
    return _clients.size();
}

const std::set<int>& Channel::getClientFds() const
{
    return _clients;
}

// Getters et setters pour les modes

void Channel::setInviteOnly(bool inviteOnly) {
    _inviteOnly = inviteOnly;
}

bool Channel::getInviteOnly() const {
    return _inviteOnly;
}

void Channel::setTopicRestricted(bool topicRestricted) {
    _topicRestricted = topicRestricted;
}

bool Channel::getTopicRestricted() const {
    return _topicRestricted;
}

void Channel::setKey(const std::string& key) {
    _key = key;
}

const std::string& Channel::getKey() const {
    return _key;
}

void Channel::setUserLimit(int limit) {
    _userLimit = limit;
}

int Channel::getUserLimit() const {
    return _userLimit;
}