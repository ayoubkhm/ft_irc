#pragma once

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <poll.h>
#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include "Client.hpp"
#include "Channel.hpp"

class Server
{
public:
    Server(int port, const std::string &password, struct tm *timeinfo);
    ~Server();
    Server(Server const& copy);
    Server &operator=(Server const& copy);

    void run();

    //Getters
    int const& getPort() const;
    int getFdByNickname(const std::string &nickname);
    std::string getDateTime() const;
    Client* getClientByFd(int fd);
    Channel* getChannelByName(const std::string& channelName);

    //Setters
    void setDatetime(struct tm *timeinfo);

    //Méthode channels
    void addChannel(const std::string &channelName);
    void removeChannel(const std::string &channelName);
    void joinChannel(int fd, const std::string &channelName);
    void broadcastToChannel(const std::string &channelName, const std::string &message, int senderFd);

    //Méthode clients
    void kickClient(int fd, const std::string &channelName, int targetFd);
    bool checkDuplicateClient(std::string const nickClient);
    void removeClient(int fd, size_t index);

private:
    int server_fd;
    int port;
    std::string password;
    std::string _datetime;
    std::vector<struct pollfd> pollfds;
    std::map<int, Client*> _ClientBook;  // Clé = FD, valeur = Client*
    std::map<std::string, Channel> _Channels;

    void initServer();
    void handleNewConnection();
    void handleClientMessage(size_t index);
};
