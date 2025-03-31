#pragma once
#include <iostream>
#include <poll.h>
#include <sys/socket.h>
#include <string>
#include <unistd.h> // pour close()

enum ClientState {
    WAITING_FOR_PASS,
    WAITING_FOR_NICK,
    WAITING_FOR_USER,
    REGISTERED
};

class Client {
private:
    int _fd;
    unsigned int _id;            // Nouvel identifiant unique pour le client
    int _port;
    bool _authenticated;
    bool _welcomeSent;
    std::string _Nickname;
    std::string _Username;
    std::string _Host;
    std::string _readBuffer;
    std::string _expectedPassword; // Mot de passe attendu, fourni par le serveur
    ClientState _state;

    static unsigned int s_nextId; // Compteur statique pour générer les IDs uniques

public:
    // Constructeur avec mot de passe attendu
    Client(int fd, const std::string &expectedPassword);
    ~Client();
    Client(Client const& copy);
    Client &operator=(Client const& copy);
    
    //Getters de client
    int getFd() const;
    int getPort() const;
    std::string getNickname() const;
    std::string getUsername() const;
    std::string getHostname() const;
    std::string getExpectedPassword() const;
    unsigned int getId() const;    // Accesseur pour l'identifiant unique
    ClientState getState() const { return _state; }
    
    //Setters de client
    void setNickname(std::string &Nickname);
    void setUsername(std::string &Username);
    void setState(ClientState newState) { _state = newState; }
    void setWelcomeReceived(bool val) { _welcomeSent = val; }
    
    // La méthode authenticate compare le mot de passe fourni avec celui stocké
    void authenticate(const std::string &password);
    bool isAuthenticated() const;

    //Méthodes pour welcome
    bool hasReceivedWelcome() const { return _welcomeSent; }

    //Méthodes pour les Ctrl + D
    void appendToBuffer(const std::string& data);
    bool hasCompleteCommand() const;
    std::string extractNextCommand();
    std::string getBuffer() const { return _readBuffer; };
    void clearBuffer() { _readBuffer.clear();};

};
