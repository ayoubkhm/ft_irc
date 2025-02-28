#include "Server.hpp"
#include <iostream>
#include <csignal>
#include <cstdlib>

// Flag global pour la gestion des signaux
volatile bool g_running = true;

void signal_handler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        std::cout << "\nSignal de fermeture reçu, arrêt du serveur...\n";
        g_running = false;
    }
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <password>\n";
        return 1;
    }
    // Installation du handler pour SIGINT et SIGTERM
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    
    int port = std::atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        std::cerr << "Numéro de port invalide.\n";
        return 1;
    }
    std::string password = argv[2];
    
    // Création et lancement du serveur
    Server server(port, password);
    try
    {
        server.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    std::cout << "Serveur arrêté.\n";
    return 0;
}
