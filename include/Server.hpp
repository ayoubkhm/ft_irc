#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <poll.h>

class Server
{
public:
    Server(int port, const std::string &password);
    ~Server();
    void run();
private:
    int server_fd;
    std::vector<struct pollfd> pollfds;
    int port;
    std::string password;

    void initServer();
    void handleNewConnection();
    void handleClientMessage(size_t index);
};

#endif
