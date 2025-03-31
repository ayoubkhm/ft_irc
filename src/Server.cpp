#include "Server.hpp"
#include "Command.hpp"
#include "IRCUtils.hpp"


extern volatile bool g_running;

Server::Server(int port, const std::string &password, struct tm *timeinfo)
    : server_fd(-1), port(port), password(password)
{
    initServer();
    this->setDatetime(timeinfo);
}

Server::~Server()
{
    // Ferme toutes les sockets du poll
    for (size_t i = 0; i < pollfds.size(); ++i)
    {
        close(pollfds[i].fd);
    }
    
    // Libère les clients
    for (std::map<int, Client*>::iterator it = _ClientBook.begin(); it != _ClientBook.end(); ++it)
    {
        delete it->second;
    }
}

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
        _ClientBook = other._ClientBook;  // shallow copy
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
    // displayRegistrationInstructions(_ClientBook[client_fd]);
}

void Server::removeClient(int fd, size_t index)
{
    std::cout << "Déconnexion du client FD " << fd << ".\n";
    close(fd);

    // Supprime le client du vecteur pollfds
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
    char buffer[1024];
    int bytes_read = read(pollfds[index].fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read < 0)
    {
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
        return;
    
    Channel& channel = it->second;
    
    // On récupère l'ID du client émetteur
    unsigned int senderId = 0;
    Client* sender = getClientByFd(senderFd);
    if (sender)
        senderId = sender->getId();
    
    // Parcours de tous les IDs clients dans le channel
    const std::set<unsigned int>& clientIds = channel.getClientIds();
    for (std::set<unsigned int>::const_iterator itId = clientIds.begin(); itId != clientIds.end(); ++itId)
    {
        if (*itId == senderId)
            continue;
        
        // Recherche du client correspondant dans _ClientBook
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
        // Créer le channel automatiquement
        Channel newChannel(channelName);
        _Channels.insert(std::make_pair(channelName, newChannel));
        std::cout << "Channel " << channelName << " créé automatiquement." << std::endl;
        newlyCreated = true;
        it = _Channels.find(channelName);
    }
    
    Channel& channel = it->second;
    if (!channel.isClientInChannel(clientId))
    {
        if (channel.addClient(clientId) == true)
            std::cout << "Le client avec ID " << clientId << " a rejoint le channel " << channelName << ".\n";
    }
    
    // Si le channel vient d'être créé, le rendre opérateur
    if (newlyCreated)
    {
        channel.addOperator(clientId);
        //sendResponse(client, "NOTICE * :Vous êtes le créateur du channel " + channelName + " et vous êtes opérateur.");
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

std::string Server::getDateTime () const
{
    return (_datetime);
}

void Server::setDatetime(struct tm *timeinfo)
{
	char buffer[80];

	strftime(buffer,sizeof(buffer),"%d-%m-%Y %H:%M:%S",timeinfo);
  	std::string str(buffer);

	_datetime = str;
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
    {
        return it->second;
    }
    return NULL;
}

Channel* Server::getChannelByName(const std::string& channelName)
{
    std::map<std::string, Channel>::iterator it = _Channels.find(channelName);
    if (it != _Channels.end())
    {
        return &it->second;
    }
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
        
        size_t clientCount = pollfds.size();
        size_t i = 1;
        while (i < clientCount)
        {
            if (i >= pollfds.size())
                break;
            if (pollfds[i].revents & POLLIN)
            {
                handleClientMessage(i);
            }
            i++;
        }
    }
}
