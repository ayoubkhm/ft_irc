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
    int const& getPort() const;
    void addChannel(const std::string &channelName);
    void removeChannel(const std::string &channelName);
    void joinChannel(int fd, const std::string &channelName);
    void kickClient(int fd, const std::string &channelName, int targetFd);
    std::string getDateTime() const;
    void setDatetime(struct tm *timeinfo);
    void broadcastToChannel(const std::string &channelName, const std::string &message, int senderFd);
    int getFdByNickname(const std::string &nickname);
    Client* getClientByFd(int fd);
    Channel* getChannelByName(const std::string& channelName);
    bool checkDuplicateClient(std::string const nickClient);

private:
    int server_fd;
    std::vector<struct pollfd> pollfds;
    int port;
    std::string password;
    std::string _datetime;
    std::map<int, Client*> _ClientBook;  // Clé = FD, valeur = Client*
    std::map<std::string, Channel> _Channels;

    void initServer();
    void handleNewConnection();
    void handleClientMessage(size_t index);
};
