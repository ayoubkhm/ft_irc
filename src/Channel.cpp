#include "Channel.hpp"

Channel::Channel(const std::string& channelName, size_t maxClients)
    : _name(channelName),
      _topic(""),
      _maxClients(maxClients),
      _inviteOnly(false),
      _topicRestricted(false),
      _key(""),
      _userLimit(0)
{
}

Channel::~Channel() {
}

Channel::Channel(const Channel& other)
    : _name(other._name),
      _topic(other._topic),
      _clients(other._clients),
      _operators(other._operators),
      _invitedIds(other._invitedIds),
      _maxClients(other._maxClients),
      _inviteOnly(other._inviteOnly),
      _topicRestricted(other._topicRestricted),
      _key(other._key),
      _userLimit(other._userLimit)
{
}

Channel& Channel::operator=(const Channel& other) {
    if (this != &other) {
        _name         = other._name;
        _topic        = other._topic;
        _clients      = other._clients;
        _operators    = other._operators;
        _invitedIds   = other._invitedIds;
        _maxClients   = other._maxClients;
        _inviteOnly   = other._inviteOnly;
        _topicRestricted = other._topicRestricted;
        _key          = other._key;
        _userLimit    = other._userLimit;
    }
    return *this;
}

void Channel::addClient(unsigned int clientId)
{
    if (_clients.size() >= _maxClients)
    {
        std::cerr << "Le channel " << _name << " est plein !" << std::endl;
        return;
    }
    _clients.insert(clientId);
}

void Channel::addInvitedClient(unsigned int clientId)
{
    _invitedIds.insert(clientId);
}

void Channel::removeInvitedClient(unsigned int clientId)
{
    _invitedIds.erase(clientId);
}

void Channel::removeClient(unsigned int clientId)
{
    std::set<unsigned int>::iterator it = _clients.find(clientId);
    if (it != _clients.end())
    {
        _clients.erase(it);
        std::cout << "Client avec ID " << clientId << " a été expulsé du channel." << std::endl;
    }
}

bool Channel::isClientInChannel(unsigned int clientId) const
{
    return _clients.find(clientId) != _clients.end();
}

bool Channel::isOperator(unsigned int clientId) const
{
    return _operators.find(clientId) != _operators.end();
}

bool Channel::isClientInvited(unsigned int clientId) const
{
    return _invitedIds.find(clientId) != _invitedIds.end();
}

void Channel::addOperator(unsigned int clientId)
{
    _operators.insert(clientId);
    std::cout << "Client avec ID " << clientId << " est maintenant opérateur." << std::endl;
}

void Channel::removeOperator(unsigned int clientId)
{
    _operators.erase(clientId);
    std::cout << "Client avec ID " << clientId << " n'est plus opérateur." << std::endl;
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

size_t Channel::getMaxClients() const
{
    return _maxClients;
}

void Channel::printClients() const
{
    std::cout << "Clients dans le channel " << _name << " :" << std::endl;
    for (std::set<unsigned int>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        std::cout << "- Client avec ID : " << *it << std::endl;
    }
}

size_t Channel::getClientCount() const
{
    return _clients.size();
}

const std::set<unsigned int>& Channel::getClientIds() const
{
    return _clients;
}

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
