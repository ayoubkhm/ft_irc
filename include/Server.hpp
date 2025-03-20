#pragma once

#include <string>
#include <vector>
#include <map>
#include <poll.h>
#include <unistd.h>
#include "Client.hpp"
#include "Channel.hpp"

class Server
{
public:
    Server(int port, const std::string &password);
    ~Server();
    void run();
    int const& getPort() const;
    void addChannel(const std::string &channelName);
    void removeChannel(const std::string &channelName);
    void joinChannel(int fd, const std::string &channelName);
    void kickClient(int fd, const std::string &channelName, int target);
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
    std::map<int, Client*> _ClientBook;
    std::map<std::string, Channel> _Channels;

    void initServer();
    void handleNewConnection();
    void handleClientMessage(size_t index);
};
