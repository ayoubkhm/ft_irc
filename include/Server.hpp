#ifndef SERVER_HPP
#define SERVER_HPP

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
    /*forme canonique pour les class les gars
    Server(Server const& copy);
    Server &operator=(Server const& copy);*/
    void run();
    int const& getPort() const;
private:
    int server_fd;
    std::vector<struct pollfd>& pollfds;
    int port;
    std::string password;
    std::map<int, Client*> _ClientBook;
    std::map<std::string, Channel>	_Channels;

    void initServer();
    void handleNewConnection();
    void handleClientMessage(size_t index);
};
/*addclient fonction();
delclient();
addchannels();
delchannels();*/
#endif
