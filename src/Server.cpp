#include "Server.hpp"
#include "Command.hpp"
#include "IRCUtils.hpp"
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern volatile bool g_running;

Server::Server(int port, const std::string &password, struct tm *timeinfo)
    : server_fd(-1), port(port), password(password)
{
    initServer();
    this->setDatetime(timeinfo);
}

Server::~Server()
{
    // On ferme toutes les sockets présentes dans pollfds (sauf STDIN)
    for (size_t i = 0; i < pollfds.size(); ++i)
    {
        if (pollfds[i].fd != STDIN_FILENO)
            close(pollfds[i].fd);
    }
    
    // Libération des clients
    for (std::map<int, Client*>::iterator it = _ClientBook.begin(); it != _ClientBook.end(); ++it)
    {
        delete it->second;
    }
}

// Constructeur de copie et opérateur d'affectation (attention : copie superficielle)
Server::Server(const Server &other)
: server_fd(other.server_fd),
  port(other.port),
  password(other.password),      
  pollfds(other.pollfds),        
  _ClientBook(other._ClientBook),
  _Channels(other._Channels)
{
}  

Server& Server::operator=(const Server &other) {
    if (this != &other) {
        server_fd = other.server_fd;
        port = other.port;
        password = other.password;
        pollfds = other.pollfds;
        _ClientBook = other._ClientBook;  // copie superficielle
        _Channels = other._Channels;
    }
    return *this;
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
    
    // On ajoute la socket serveur dans pollfds (index 0)
    struct pollfd pfd;
    pfd.fd = server_fd;
    pfd.events = POLLIN;
    pollfds.push_back(pfd);
    
    // On ajoute STDIN_FILENO dans pollfds pour gérer Ctrl+D côté serveur (index 1)
    struct pollfd stdin_pfd;
    stdin_pfd.fd = STDIN_FILENO;
    stdin_pfd.events = POLLIN;
    pollfds.push_back(stdin_pfd);
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

void Server::removeClient(int fd, size_t index)
{
    std::cout << "Déconnexion du client FD " << fd << ".\n";
    close(fd);

    // Supprime la socket du client du vecteur pollfds
    if (index < pollfds.size())
        pollfds.erase(pollfds.begin() + index);

    // Supprime le client du registre _ClientBook
    std::map<int, Client*>::iterator it = _ClientBook.find(fd);
    if (it != _ClientBook.end())
    {
        delete it->second;
        _ClientBook.erase(it);
    }
}

void Server::handleClientMessage(size_t index)
{
    int fd = pollfds[index].fd;
    char buffer[1024];
    int bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        std::cerr << "Erreur de lecture sur FD " << fd << ": " << strerror(errno) << "\n";
        removeClient(fd, index);
        return;
    }
    else if (bytes_read == 0)
    {
        // En cas d'EOF, on jette toute commande incomplète
        _clientBuffers.erase(fd);
        std::cout << "Client FD " << fd << " déconnecté (EOF).\n";
        removeClient(fd, index);
        return;
    }
    
    buffer[bytes_read] = '\0';
    _clientBuffers[fd] += std::string(buffer);
    
    // Traiter uniquement les lignes complètes (terminées par '\n')
    size_t pos;
    while ((pos = _clientBuffers[fd].find('\n')) != std::string::npos)
    {
        std::string line = _clientBuffers[fd].substr(0, pos + 1);
        _clientBuffers[fd].erase(0, pos + 1);
        
        // Retirer le '\r' en fin de ligne si nécessaire
        if (!line.empty() && line[line.size() - 1] == '\n')
        {
            if (line.size() > 1 && line[line.size() - 2] == '\r')
                line.erase(line.size() - 2, 1);
        }
        
        // On traite la commande seulement si elle n'est pas vide
        if (!line.empty())
            parseAndDispatch(this, getClientByFd(fd), line);
    }
}

void Server::broadcastToChannel(const std::string &channelName, const std::string &message, int senderFd)
{
    std::map<std::string, Channel>::iterator it = _Channels.find(channelName);
    if (it == _Channels.end())
        return;
    
    Channel& channel = it->second;
    
    // On récupère l'ID du client émetteur
    unsigned int senderId = 0;
    Client* sender = getClientByFd(senderFd);
    if (sender)
        senderId = sender->getId();
    
    const std::set<unsigned int>& clientIds = channel.getClientIds();
    for (std::set<unsigned int>::const_iterator itId = clientIds.begin(); itId != clientIds.end(); ++itId)
    {
        if (*itId == senderId)
            continue;
        for (std::map<int, Client*>::iterator itClient = _ClientBook.begin(); itClient != _ClientBook.end(); ++itClient)
        {
            if (itClient->second->getId() == *itId)
            {
                std::string resp = message + "\r\n";
                write(itClient->second->getFd(), resp.c_str(), resp.size());
            }
        }
    }
}

void Server::addChannel(const std::string& channelName)
{
    if (_Channels.find(channelName) == _Channels.end())
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
    Client* client = getClientByFd(fd);
    if (!client)
        return;
    unsigned int clientId = client->getId();
    
    std::map<std::string, Channel>::iterator it = _Channels.find(channelName);
    bool newlyCreated = false;
    if (it == _Channels.end())
    {
        Channel newChannel(channelName);
        _Channels.insert(std::make_pair(channelName, newChannel));
        std::cout << "Channel " << channelName << " créé automatiquement." << std::endl;
        newlyCreated = true;
        it = _Channels.find(channelName);
    }
    
    Channel& channel = it->second;
    if (!channel.isClientInChannel(clientId))
    {
        if (channel.addClient(clientId))
            std::cout << "Le client avec ID " << clientId << " a rejoint le channel " << channelName << ".\n";
    }
    
    if (newlyCreated)
    {
        channel.addOperator(clientId);
    }
}

void Server::kickClient(int fd, const std::string& channelName, int targetFd)
{
    Client* client = getClientByFd(fd);
    Client* targetClient = getClientByFd(targetFd);
    if (!client || !targetClient)
        return;
    
    unsigned int clientId = client->getId();
    unsigned int targetId = targetClient->getId();
    
    if (fd == targetFd)
    {
        std::cout << "Vous ne pouvez pas vous kick vous-même !\n";
        return;
    }
    
    std::map<std::string, Channel>::iterator it = _Channels.find(channelName);
    if (it == _Channels.end())
    {
        std::cout << "Le channel " << channelName << " n'existe pas.\n";
        return;
    }
    
    Channel& channel = it->second;
    if (!channel.isOperator(clientId))
    {
        std::cout << "Vous devez être opérateur pour expulser un client.\n";
        return;
    }
    if (!channel.isClientInChannel(targetId))
    {
        std::cout << "Le client avec ID " << targetId << " n'est pas dans ce channel.\n";
        return;
    }
    channel.removeClient(targetId);
    std::cout << "Le client avec ID " << targetId << " a été expulsé du channel " << channelName << ".\n";
}

std::string Server::getDateTime() const
{
    return _datetime;
}

void Server::setDatetime(struct tm *timeinfo)
{
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", timeinfo);
    _datetime = std::string(buffer);
}

int Server::getFdByNickname(const std::string &nickname)
{
    for (std::map<int, Client*>::iterator it = _ClientBook.begin(); it != _ClientBook.end(); ++it)
    {
        if (it->second->getNickname() == nickname)
            return it->first;
    }
    return -1; // Pas trouvé
}

Client* Server::getClientByFd(int fd)
{
    std::map<int, Client*>::iterator it = _ClientBook.find(fd);
    if (it != _ClientBook.end())
        return it->second;
    return NULL;
}

Channel* Server::getChannelByName(const std::string& channelName)
{
    std::map<std::string, Channel>::iterator it = _Channels.find(channelName);
    if (it != _Channels.end())
        return &it->second;
    return NULL;
}

bool Server::checkDuplicateClient(std::string const nickClient)
{
    for (std::map<int, Client*>::iterator it = _ClientBook.begin(); it != _ClientBook.end(); ++it)
    {
        if (it->second->getNickname() == nickClient)
            return true;
    }
    return false;
}

void Server::run()
{
    while (g_running)
    {
        int poll_count = poll(&pollfds[0], pollfds.size(), 1000);
        if (poll_count < 0)
        {
            if (errno == EINTR)
                continue;
            std::cerr << "Erreur poll: " << strerror(errno) << "\n";
            break;
        }
        
        // Gestion de la socket serveur (index 0)
        if (pollfds[0].revents & POLLIN)
        {
            handleNewConnection();
        }
        
        // Gestion de STDIN pour le serveur (index 1)
        if (pollfds[1].revents & POLLIN)
        {
            char input_buffer[1024];
            int bytes = read(STDIN_FILENO, input_buffer, sizeof(input_buffer) - 1);
            if (bytes <= 0)
            {
                std::cout << "Ctrl+D détecté sur le serveur. Arrêt...\n";
                g_running = false;
            }
            else
            {
                input_buffer[bytes] = '\0';
                std::cout << "Commande serveur: " << input_buffer;
                // Traitement éventuel des commandes serveur...
            }
        }
        
        // Gestion des clients (indices ≥ 2)
        for (int i = pollfds.size() - 1; i >= 2; --i)
        {
            if (pollfds[i].revents & POLLIN)
            {
                handleClientMessage(i);
            }
        }
    }
    
    // Fermer proprement la socket serveur avant la destruction
    if (server_fd != -1)
    {
        close(server_fd);
        server_fd = -1;
        // On retire la socket serveur du vecteur pollfds
        if (!pollfds.empty() && pollfds[0].fd != STDIN_FILENO)
            pollfds.erase(pollfds.begin());
    }
}
