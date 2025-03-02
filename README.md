Fonctionnalités
Commandes IRC de base :

PASS : Authentifie le client avec un mot de passe. ✅
NICK : Définit le pseudo du client. ✅
USER : Définit le nom d'utilisateur du client. ✅
JOIN : Permet de rejoindre (ou de créer automatiquement) un channel. ✅
PRIVMSG : Envoie un message à un channel (le support pour l'envoi de messages privés n'est pas encore implémenté). 💀💀💀
KICK : Expulse un client d'un channel. ✅
INVITE : Invite un client à rejoindre un channel. ✅
TOPIC et MODE : Commandes actuellement en version "dummy" qui devront être complétées. 💀💀💀


Gestion des channels :

Création automatique d'un channel si utilisateur tente de join un channel qui n'existe pas. ✅
Attribution des droits d'opérateur au créateur du channel. ✅
Possibilité de bannir/débannir des clients. ➡️ Ce n'est pas demandé par le sujet, mais j'ai vu que l'un d'entre vous (je crois Gino) a ajouté ces fonctionnalités


Gestion des clients :

Authentification avec mot de passe. ✅
Connexions gérées en mode non bloquant. ✅
Utilisation de poll pour le multiplexage des entrées/sorties. ✅


Gestion des signaux :

Arrêt propre du serveur sur réception de SIGINT ou SIGTERM (Ctrl+C). ✅
Compilation ✅



Exécution 
Après la compilation, lancez le serveur avec :

./ircserv <port> <password>


Exemple pour lancer le serveur sur le port 6667 avec le mot de passe "mySecret" :

./ircserv 6667 mySecret




Connexion au serveur
Vous pouvez utiliser n'importe quel client IRC ou des outils comme telnet ou netcat pour vous connecter. Par exemple :
telnet localhost 6667

Une fois connecté, vous devrez envoyer les commandes IRC (PASS, NICK, USER, etc.) pour vous enregistrer et interagir avec le serveur.

Commandes supportées
PASS <password> : Authentifie le client. ✅
NICK <nickname> : Définit le pseudo du client. ✅
USER <username> ... : Définit le nom d'utilisateur du client. (2 DEUX ARGUMENTS MINIMUM) ✅
JOIN <channel> : Permet de rejoindre ou de créer un channel. ✅
PRIVMSG <target> <message> : Envoie un message à un channel (les messages privés ne sont pas encore implémentés).
KICK <channel> <nickname> : Expulse un client d'un channel. ✅
INVITE <channel> <nickname> : Invite un client à rejoindre un channel. ✅
TOPIC <channel> <topic> : (Non implémenté)
MODE <channel> ... : (Non implémenté)



Points à améliorer
Implémentation de commandes supplémentaires :

Messages privés (PRIVMSG) : Permettre l'envoi de messages directement à un utilisateur.
PART et QUIT : Gérer la sortie des utilisateurs d'un channel ou du serveur. --> C'est pas demandé par le sujet, mais c'est notre IA qui le propose si on veut aller au delà.
TOPIC et MODE : Compléter ces commandes pour permettre la gestion du topic et des modes du channel.


Robustesse :

Améliorer la validation des entrées et la gestion des erreurs. --> J'ai déjà pas mal testé mais on est jamais trop surs.
