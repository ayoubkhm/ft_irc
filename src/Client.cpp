#include "Client.hpp"
#include <unistd.h> // pour close()

// Constructeur principal avec mot de passe attendu
Client::Client(int fd, const std::string &expectedPassword)
    : _fd(fd), _port(0), _Nickname(""), _Username(""), _Host(""), _authenticated(false),
      _expectedPassword(expectedPassword)
{
}

// Destructeur
Client::~Client()
{
    close(_fd);
}

// Constructeur de copie
Client::Client(Client const & copy)
    : _fd(copy._fd),
      _port(copy._port),
      _Nickname(copy._Nickname),
      _Username(copy._Username),
      _Host(copy._Host),
      _authenticated(copy._authenticated),
      _expectedPassword(copy._expectedPassword)
{
}

// Opérateur d'affectation
Client & Client::operator=(Client const & other)
{
    if (this != &other)
    {
        _fd = other._fd;
        _port = other._port;
        _Nickname = other._Nickname;
        _Username = other._Username;
        _Host = other._Host;
        _authenticated = other._authenticated;
        _expectedPassword = other._expectedPassword;
    }
    return *this;
}

int Client::getFd() const
{
    return _fd;
}

int Client::getPort() const
{
    return _port;
}

std::string Client::getNickname() const
{
    return _Nickname;
}

std::string Client::getUsername() const
{
    return _Username;
}

std::string Client::getHostname() const
{
    return _Host;
}

void Client::setNickname(std::string &Nickname)
{
    _Nickname = Nickname;
}

void Client::setUsername(std::string &Username)
{
    _Username = Username;
}

// La méthode authenticate compare le mot de passe fourni avec celui stocké dans _expectedPassword
void Client::authenticate(const std::string &password)
{
    if (password == _expectedPassword)
    {
        _authenticated = true;
    }
}

bool Client::isAuthenticated() const
{
    return _authenticated;
}

std::string Client::getExpectedPassword() const
{
    return _expectedPassword;
}
