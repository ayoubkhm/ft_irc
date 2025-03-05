#include "Server.hpp"
#include "Command.hpp"
#include "IRCUtils.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

extern volatile bool g_running;

Server::Server(int port, const std::string &password)
    : server_fd(-1), port(port), password(password)
{
    initServer();
}

Server::~Server()
{
    size_t i = 0;
    while (i < pollfds.size())
    {
        close(pollfds[i].fd);
        i++;
    }
    
    std::map<int, Client*>::iterator it = _ClientBook.begin();
    while (it != _ClientBook.end())
    {
        delete it->second;
        it++;
    }
}

void Server::initServer()
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        std::cerr << "Erreur lors de la création de la socket: " << strerror(errno) << "\n";
        exit(1);
    }
    
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        std::cerr << "Erreur setsockopt: " << strerror(errno) << "\n";
        close(server_fd);
        exit(1);
    }
    
    if (fcntl(server_fd, F_SETFL, O_NONBLOCK) < 0)
    {
        std::cerr << "Erreur lors du passage en mode non bloquant: " << strerror(errno) << "\n";
        close(server_fd);
        exit(1);
    }
    
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_fd, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0)
    {
        std::cerr << "Erreur de bind: " << strerror(errno) << "\n";
        close(server_fd);
        exit(1);
    }
    
    if (listen(server_fd, SOMAXCONN) < 0)
    {
        std::cerr << "Erreur de listen: " << strerror(errno) << "\n";
        close(server_fd);
        exit(1);
    }
    
    std::cout << "Serveur initialisé sur le port " << port << " avec le mot de passe '" << password << "'\n";
    
    struct pollfd pfd;
    pfd.fd = server_fd;
    pfd.events = POLLIN;
    pollfds.push_back(pfd);
}

void Server::handleNewConnection()
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);
    
    if (client_fd < 0)
    {
        std::cerr << "Erreur accept: " << strerror(errno) << "\n";
        return;
    }
    
    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0)
    {
        std::cerr << "Erreur fcntl sur client_fd: " << strerror(errno) << "\n";
        close(client_fd);
        return;
    }
    
    struct pollfd client_pfd;
    client_pfd.fd = client_fd;
    client_pfd.events = POLLIN;
    pollfds.push_back(client_pfd);
    
    std::cout << "Nouvelle connexion, FD = " << client_fd << "\n";
    _ClientBook[client_fd] = new Client(client_fd, password);    
}


void Server::handleClientMessage(size_t index)
{
    char buffer[1024];
    int bytes_read = read(pollfds[index].fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read < 0)
    {
        // Si aucune donnée n'est disponible, on ne fait rien (EAGAIN)
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return;
        }
        std::cerr << "Erreur de lecture sur FD " << pollfds[index].fd << ": " << strerror(errno) << "\n";
        int fd = pollfds[index].fd;
        close(fd);
        pollfds.erase(pollfds.begin() + index);
        
        std::map<int, Client*>::iterator it = _ClientBook.find(fd);
        if (it != _ClientBook.end())
        {
            delete it->second;
            _ClientBook.erase(it);
        }
        return;
    }
    else if (bytes_read == 0)
    {
        std::cout << "Client FD " << pollfds[index].fd << " déconnecté (EOF).\n";
        int fd = pollfds[index].fd;
        close(fd);
        pollfds.erase(pollfds.begin() + index);
        
        std::map<int, Client*>::iterator it = _ClientBook.find(fd);
        if (it != _ClientBook.end())
        {
            delete it->second;
            _ClientBook.erase(it);
        }
        return;
    }
    
    buffer[bytes_read] = '\0';
    std::cout << "Message reçu de FD " << pollfds[index].fd << ": " << buffer;
    
    int fd = pollfds[index].fd;
    Client* client = NULL;
    
    std::map<int, Client*>::iterator it = _ClientBook.find(fd);
    if (it != _ClientBook.end())
    {
        client = it->second;
    }
    
    if (client != NULL)
    {
        parseAndDispatch(this, client, std::string(buffer));
    }
    else
    {
        std::cerr << "Erreur: client non trouvé pour FD " << fd << "\n";
    }
}

void Server::broadcastToChannel(const std::string &channelName, const std::string &message, int senderFd)
{
    std::map<std::string, Channel>::iterator it = _Channels.find(channelName);
    if (it == _Channels.end())
    {
        return;
    }
    
    Channel& channel = it->second;
    const std::set<int>& fds = channel.getClientFds();
    
    std::set<int>::const_iterator itFd = fds.begin();
    while (itFd != fds.end())
    {
        if (*itFd != senderFd)
        {
            std::map<int, Client*>::iterator itClient = _ClientBook.find(*itFd);
            if (itClient != _ClientBook.end())
            {
                Client* client = itClient->second;
                std::string resp = message + "\r\n";
                write(client->getFd(), resp.c_str(), resp.size());
            }
        }
        itFd++;
    }
}

void Server::addChannel(const std::string& channelName)
{
    std::map<std::string, Channel>::iterator it = _Channels.find(channelName);
    if (it == _Channels.end())
    {
        Channel newChannel(channelName);
        _Channels.insert(std::make_pair(channelName, newChannel));
        std::cout << "Channel créé : " << channelName << "\n";
    }
    else
    {
        std::cout << "Le channel " << channelName << " existe déjà.\n";
    }
}

void Server::removeChannel(const std::string& channelName)
{
    std::map<std::string, Channel>::iterator it = _Channels.find(channelName);
    if (it != _Channels.end())
    {
        _Channels.erase(it);
        std::cout << "Channel " << channelName << " supprimé.\n";
    }
    else
    {
        std::cout << "Le channel " << channelName << " n'existe pas.\n";
    }
}

void Server::joinChannel(int fd, const std::string& channelName)
{
    std::map<std::string, Channel>::iterator it = _Channels.find(channelName);
    bool newlyCreated = false;
    
    if (it == _Channels.end())
    {
        // Créer le channel automatiquement
        Channel newChannel(channelName);
        _Channels.insert(std::make_pair(channelName, newChannel));
        std::cout << "Channel " << channelName << " créé automatiquement." << std::endl;
        newlyCreated = true;
        it = _Channels.find(channelName);
    }
    
    Channel& channel = it->second;
    channel.addClient(fd);
    std::cout << "Le client avec fd " << fd << " a rejoint le channel " << channelName << ".\n";
    
    // Si le channel vient d'être créé, confère les droits d'opérateur au créateur
    if (newlyCreated)
    {
        channel.addOperator(fd);
        std::map<int, Client*>::iterator itClient = _ClientBook.find(fd);
        if (itClient != _ClientBook.end())
        {
            Client* client = itClient->second;
            sendResponse(client, "NOTICE * :Vous êtes le créateur du channel " + channelName + " et vous êtes opérateur.");
        }
    }
}


void Server::kickClient(int fd, const std::string& channelName, int target)
{
    std::map<std::string, Channel>::iterator it = _Channels.find(channelName);
    if (it == _Channels.end())
    {
        std::cout << "Le channel " << channelName << " n'existe pas.\n";
        return;
    }
    
    if (fd == target)
    {
        std::cout << "Vous ne pouvez pas vous kick vous-même !\n";
        return;
    }
    
    Channel& channel = it->second;
    if (!channel.isOperator(fd))
    {
        std::cout << "Vous devez être opérateur pour expulser un client.\n";
        return;
    }
    if (!channel.isClientInChannel(target))
    {
        std::cout << "Le client avec fd " << target << " n'est pas dans ce channel.\n";
        return;
    }
    channel.removeClient(target);
    std::cout << "Le client avec fd " << target << " a été expulsé du channel " << channelName << ".\n";
}

void Server::run()
{
    while (g_running)
    {
        int poll_count = poll(&pollfds[0], pollfds.size(), 1000);
        if (poll_count < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            std::cerr << "Erreur poll: " << strerror(errno) << "\n";
            break;
        }
        
        // Gérer les événements sur la socket serveur (toujours à l'index 0)
        if (pollfds[0].revents & POLLIN)
        {
            handleNewConnection();
        }
        
        // Sauvegarder la taille initiale des clients
        size_t clientCount = pollfds.size();
        size_t i = 1;
        while (i < clientCount)
        {
            // Si l'élément a été supprimé, on arrête la boucle
            if (i >= pollfds.size())
            {
                break;
            }
            if (pollfds[i].revents & POLLIN)
            {
                handleClientMessage(i);
            }
            i++;
        }
    }
}

int Server::getFdByNickname(const std::string &nickname)
{
    std::map<int, Client*>::iterator it = _ClientBook.begin();
    while (it != _ClientBook.end())
    {
        if (it->second->getNickname() == nickname)
        {
            return it->first;
        }
        it++;
    }
    return -1; // Pas trouvé
}

Client* Server::getClientByFd(int fd)
{
    std::map<int, Client*>::iterator it = _ClientBook.find(fd);
    if (it != _ClientBook.end())
    {
        return it->second;
    }
    else
    {
        return NULL;
    }
}
