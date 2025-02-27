#include "Server.hpp"
#include "Command.hpp"
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
    for (size_t i = 0; i < pollfds.size(); ++i)
    {
        close(pollfds[i].fd);
    }
}


// Initialisation de la socket serveur
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
    if (bind(server_fd, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
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
    
    // Ajout de la socket serveur dans le vecteur pollfds
    struct pollfd pfd;
    pfd.fd = server_fd;
    pfd.events = POLLIN;
    pollfds.push_back(pfd);
}

// Accepter une nouvelle connexion client
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
    // Envoi d'un message de bienvenue
    const char *welcome = "Welcome to the IRC server!\r\n";
    write(client_fd, welcome, std::strlen(welcome));
}



// Gérer un message reçu depuis un client
void Server::handleClientMessage(size_t index)
{
    char buffer[1024];
    int bytes_read = read(pollfds[index].fd, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0)
    {
        std::cout << "Client FD " << pollfds[index].fd << " déconnecté.\n";
        close(pollfds[index].fd);
        pollfds.erase(pollfds.begin() + index);
        return;
    }
    buffer[bytes_read] = '\0';
    std::cout << "Message reçu de FD " << pollfds[index].fd << ": " << buffer;
    // Utilisation du module Command pour parser et dispatcher la commande
    parseAndDispatch(pollfds[index].fd, std::string(buffer));
}



void Server::run()
{
    while (g_running)
    {
        int poll_count = poll(&pollfds[0], pollfds.size(), 1000); // Timeout de 1000 ms
        if (poll_count < 0)
        {
            if (errno == EINTR)
                continue;
            std::cerr << "Erreur poll: " << strerror(errno) << "\n";
            break;
        }
        // Vérification de la socket serveur pour de nouvelles connexions
        if (pollfds[0].revents & POLLIN)
        {
            handleNewConnection();
        }
        // Parcours des clients
        for (size_t i = 1; i < pollfds.size(); ++i)
        {
            if (pollfds[i].revents & POLLIN)
            {
                handleClientMessage(i);
            }
        }
    }
}