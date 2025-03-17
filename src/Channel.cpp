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

bool Channel::isClientInChannel(int fd) const
{
    return _clients.find(fd) != _clients.end();
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

void Channel::parsingModeParam(std::string option, Client* client)
{
    std::string s = "itkol";
    char sign = '=';
    for (int i = 0; i < option.size(); i++)
    {
        if (option[i] == '-' || option[i] == '+') {
            sign = option[i];
            continue;
        }
        if (std::find(s.begin(), s.end(), option[i]) != s.end())
        {
            switch (option[i]) {
                case 'i': // Mode invite-only
                    if (sign == '+') {
                        // Activer le mode invite-only pour le channel
                        // Exemple: channel->setInviteOnly(true);
                    } else {
                        // Désactiver le mode invite-only
                        // Exemple: channel->setInviteOnly(false);
                    }
                    break;
                case 't': // Mode topic restricted
                    if (sign == '+') {
                        // Restreindre la modification du topic aux opérateurs
                        // Exemple: channel->setTopicRestricted(true);
                    } else {
                        // Permettre à tous de modifier le topic
                        // Exemple: channel->setTopicRestricted(false);
                    }
                    break;
                case 'k': // Mode clé (password)
                    if (sign == '+') {
                        // Pour activer le mode +k, il faut récupérer en supplément la clé
                        // Exemple: channel->setKey(clé);
                    } else {
                        // Désactiver la clé du channel
                        // Exemple: channel->setKey("");
                    }
                    break;
                case 'o': // Donner ou retirer les privilèges d'opérateur
                    if (sign == '+') {
                        // Donner le statut d'opérateur à un utilisateur
                        // Exemple: channel->addOperator(targetNickname);
                    } else {
                        // Retirer le statut d'opérateur à un utilisateur
                        // Exemple: channel->removeOperator(targetNickname);
                    }
                    break;
                case 'l': // Limite du nombre d'utilisateurs
                    if (sign == '+') {
                        // Activer une limite sur le nombre d'utilisateurs dans le channel
                        // Exemple: channel->setUserLimit(limitValue);
                    } else {
                        // Désactiver la limite
                        // Exemple: channel->setUserLimit(0);
                    }
                    break;
                default:
                    break;
            }
        }
        else
            continue;
    }
}