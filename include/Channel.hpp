#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <set>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include "Client.hpp"  // Assure-toi d'avoir la classe Client incluse

class Channel {
private:
    std::string name;               // Nom du channel
    std::string topic;              // Sujet du channel (TOPIC)
    std::set<int> clients;          // Clients dans le channel (représentés par leur fd)
    std::set<int> bannedClients;    // Clients bannis (représentés par leur fd)
    std::set<int> operators;        // Utilisateurs avec les droits d'admin
    std::string mode;               // Mode du channel (ex : "invite-only", "moderated", etc.)
    size_t maxClients;              // Nombre maximum de clients dans le channel

public:
    // Constructeur
    Channel(const std::string& channelName, size_t maxClients = 50, const std::string& mode = "");

    // Ajouter un client au channel
    void addClient(int fd, const std::string& username);

    // Retirer un client du channel (kick)
    void removeClient(int fd);

    // Bannir un client
    void banClient(int fd);

    // Débannir un client
    void unbanClient(int fd);

    // Vérifier si un client est dans le channel
    bool isClientInChannel(int fd) const;

    // Vérifier si un client est banni
    bool isBanned(int fd) const;

    // Vérifier si un client est admin
    bool isOperator(int fd) const;

    // Ajouter un admin
    void addOperator(int fd);

    // Retirer un admin
    void removeOperator(int fd);

    // Mettre à jour le topic
    void setTopic(const std::string& newTopic);

    // Getter pour le topic du channel
    const std::string& getTopic() const;

    // Getter pour le mode du channel
    const std::string& getMode() const;

    // Getter pour le nombre maximum de clients
    size_t getMaxClients() const;

    // Afficher les informations des clients dans le channel (juste leurs pseudo ici)
    void printClients() const;

    // Retourner le Client par son pseudo
    Client* getClientByUsername(const std::string& username);
};

#endif
