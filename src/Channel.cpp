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

void Channel::removeClient(int fd)
{
    std::set<int>::iterator it = _clients.find(fd);
    if (it != _clients.end())
    {
        _clients.erase(it);
        std::cout << "Client avec fd " << fd << " a été expulsé du channel." << std::endl;
    }
}

void Channel::banClient(int fd)
{
    _bannedClients.insert(fd);
    std::cout << "Client avec fd " << fd << " a été banni du channel." << std::endl;
}

void Channel::unbanClient(int fd)
{
    _bannedClients.erase(fd);
    std::cout << "Client avec fd " << fd << " a été débanni du channel." << std::endl;
}

bool Channel::isClientInChannel(int fd) const
{
    return _clients.find(fd) != _clients.end();
}

bool Channel::isBanned(int fd) const
{
    return _bannedClients.find(fd) != _bannedClients.end();
}

bool Channel::isOperator(int fd) const
{
    return _operators.find(fd) != _operators.end();
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
    return _mode;
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
