#include "Client.hpp"

// Initialisation du compteur statique
unsigned int Client::s_nextId = 1; // On peut démarrer à 1 par exemple

// Constructeur principal avec mot de passe attendu
Client::Client(int fd, const std::string &expectedPassword)
    : _fd(fd), _id(s_nextId++), _port(0), _authenticated(false), _welcomeSent(false), _Nickname(""), _Username(""),
     _Host(""), _readBuffer(""),_expectedPassword(expectedPassword), _state(WAITING_FOR_PASS)
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
      _id(copy._id), // on copie l'ID, car il doit rester identique à celui attribué
      _port(copy._port),
      _authenticated(copy._authenticated),
      _welcomeSent(copy._welcomeSent),
      _Nickname(copy._Nickname),
      _Username(copy._Username),
      _Host(copy._Host),
      _readBuffer(copy._readBuffer),
      _expectedPassword(copy._expectedPassword),  
      _state(copy._state)
{
}

// Opérateur d'affectation
Client & Client::operator=(Client const & other)
{
    if (this != &other)
    {
        _fd = other._fd;
        _id = other._id; // on garde l'ID d'origine
        _port = other._port;
        _Nickname = other._Nickname;
        _Username = other._Username;
        _Host = other._Host;
        _authenticated = other._authenticated;
        _expectedPassword = other._expectedPassword;
        _welcomeSent = other._welcomeSent;
        _state = other._state;
    }
    return *this;
}

int Client::getFd() const
{
    return _fd;
}

unsigned int Client::getId() const
{
    return _id;
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

void Client::appendToBuffer(const std::string& data)
{
    _readBuffer += data;
}

bool Client::hasCompleteCommand() const
{
    return _readBuffer.find("\n") != std::string::npos;
}
std::string Client::extractNextCommand()
{
    size_t pos = _readBuffer.find("\n");
    if (pos == (size_t)-1)
        return "";

    std::string command = _readBuffer.substr(0, pos); // 🔹 Prend la première commande
    _readBuffer.erase(0, pos + 1); // 🔹 Supprime la commande extraite du buffer

    if (!command.empty() && command[command.size() - 1] == '\r') 
        command.erase(command.size() - 1);

    return command;
}

