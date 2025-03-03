#pragma once
#include <iostream>
#include <poll.h>
#include <sys/socket.h>
#include <string>

class Client {
private:
    int _fd;
    int _port;
    std::string _Nickname;
    std::string _Username;
    std::string _Host;
    bool _authenticated;
    std::string _expectedPassword; // Mot de passe attendu, fourni par le serveur
    bool _welcomeSent;


public:
    // Constructeur avec mot de passe attendu
    Client(int fd, const std::string &expectedPassword);
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
    
    // La méthode authenticate compare le mot de passe fourni avec celui stocké
    void authenticate(const std::string &password);
    bool isAuthenticated() const;
    
    // Getter pour le mot de passe attendu
    std::string getExpectedPassword() const;
    bool hasReceivedWelcome() const { return _welcomeSent; }
    void setWelcomeReceived(bool val) { _welcomeSent = val; }
};
