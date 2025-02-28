#include "Client.hpp"

Client::Client(int fd, int port, std::string const &host): _fd(fd), _port(port), _Host(host) {}

Client::~Client() {close(_fd);}

int Client::getFd() const {return (_fd);}
int Client::getPort() const {return _port;}
std::string Client::getNickname() const {return _Nickname;}
std::string Client::getUsername() const {return _Username;}
std::string Client::getHostname() const {return _Host;}
void Client::setNickname(std::string &Nickname) {_Nickname = Nickname;}
void Client::setUsername(std::string &Username) {_Username = Username;}
void Client::authenticate(const std::string &password, const std::string &expectedPassword)
{
    if (password == expectedPassword)
        _authenticated = true;
    
}
bool Client::isAuthenticated() const

