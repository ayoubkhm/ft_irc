#include "Channel.hpp"

// Constructeur de Channel
Channel::Channel(const std::string& channelName, size_t maxClients, const std::string& mode): _name(channelName), _topic(""), _mode(mode), _maxClients(maxClients) 
{

}

void Channel::addClient(int fd)
{
    if (_clients.size() >= _maxClients) {
        std::cerr << "Le channel "<< this->_name << " est plein !\n";
        return;
    }
	_clients.insert(fd);
}

void Channel::removeClient(int fd) 
{
    auto it = _clients.find(fd);
    if (it != _clients.end()) {
        _clients.erase(it);
        std::cout << "Client avec fd " << fd << " a été expulsé du channel.\n";
    }
}

void Channel::banClient(int fd) 
{
    _bannedClients.insert(fd);  // Ajout du fd à la liste des clients bannis
    std::cout << "Client avec fd " << fd << " a été banni du channel.\n";
}

void Channel::unbanClient(int fd) 
{
    _bannedClients.erase(fd);
    std::cout << "Client avec fd " << fd << " a été débanni du channel.\n";
}

bool Channel::isClientInChannel(int fd) const 
{
    return _clients.find(fd) != _clients.end();  // Vérifie si le fd est dans la set des clients
}

bool Channel::isBanned(int fd) const 
{
    return _bannedClients.find(fd) != _bannedClients.end();  // Vérifie si le fd est dans la set des bannis
}

bool Channel::isOperator(int fd) const 
{
    return _operators.find(fd) != _operators.end();  // Vérifie si le fd est dans la set des opérateurs
}

void Channel::addOperator(int fd) 
{
    _operators.insert(fd);  // Ajoute le fd à la set des opérateurs
    std::cout << "Client avec fd " << fd << " est maintenant opérateur.\n";
}

void Channel::removeOperator(int fd) 
{
    _operators.erase(fd);  // Retire le fd de la set des opérateurs
    std::cout << "Client avec fd " << fd << " n'est plus opérateur.\n";
}

void Channel::setTopic(const std::string& newTopic) 
{
    _topic = newTopic;
    std::cout << "Le topic du channel " << _name << " a été mis à jour : " << _topic << "\n";
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
    std::cout << "Clients dans le channel " << _name << " :\n";
    for (const int& fd : _clients) 
	{
        std::cout << "- Client avec fd : " << fd << "\n";  // Affiche les fd des clients
    }
}

size_t Channel::getClientCount() const 
{
    return _clients.size();
}
