#pragma once
#include <iostream>
#include <poll.h>
#include <sys/socket.h>
#include <string>
#include "Server.hpp"

class Server;

class Client {
    private:
        int _fd;
        int _port;
        std::string _Nickname;
        std::string _Username;
        std::string _Host;
        bool _authenticated; 

    public:
        Client(int fd);
        ~Client();
        Client(Client const& copy);
        Client &operator=(Client const& copy);
        int getFd() const;
        int getPort() const;
        std::string getNickname() const;
        std::string getUsername() const;
        std::string getHostname() const;
        void setNickname(std::string &Nickname);
        void setUsername(std::string &Username);
        void authenticate(const std::string &password, const std::string &expectedPassword);
        bool isAuthenticated() const;
};