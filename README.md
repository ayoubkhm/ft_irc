# ft_irc - Documentation

## Fonctionnalités

### Commandes IRC de base

- **PASS** : Authentifie le client avec un mot de passe. ✅
- **NICK** : Définit le pseudo du client. ✅
- **USER** : Définit le nom d'utilisateur du client. ✅
- **JOIN** : Permet de rejoindre (ou de créer automatiquement) un channel. ✅
- **PRIVMSG** : Envoie un message à un channel (le support pour l'envoi de messages privés n'est pas encore implémenté). 💀💀💀
- **KICK** : Expulse un client d'un channel. ✅
- **INVITE** : Invite un client à rejoindre un channel. ✅
- **TOPIC** et **MODE** : Commandes actuellement en version "dummy" qui devront être complétées. 💀💀💀

### Gestion des channels

- **Création automatique d'un channel** : Si un utilisateur tente de rejoindre un channel qui n'existe pas, le channel est créé automatiquement. ✅
- **Attribution des droits d'opérateur** : Le créateur du channel reçoit les droits d'opérateur. ✅
- **Bannir/Débannir des clients** : Cette fonctionnalité n'est pas demandée par le sujet, mais certains (je crois Gino) l'ont ajoutée.

### Gestion des clients

- **Authentification avec mot de passe.** ✅
- **Connexions gérées en mode non bloquant.** ✅
- **Utilisation de `poll` pour le multiplexage des entrées/sorties.** ✅

### Gestion des signaux

- **Arrêt propre du serveur** sur réception de SIGINT ou SIGTERM (Ctrl+C). ✅
- **Compilation** : Le projet se compile correctement. ✅

## Exécution

Exemple pour lancer le serveur sur le port 6667 avec le mot de passe "mySecret" :
```bash ./ircserv 6667 mySecret```

Vous pouvez utiliser n'importe quel client IRC ou des outils comme telnet ou netcat pour vous connecter. Par exemple :

```telnet localhost 6667```
ou bien 
```nc localhost 6667```

Une fois connecté, vous devrez envoyer les commandes IRC (PASS, NICK, USER, etc.) pour vous enregistrer et interagir avec le serveur.


## Points à améliorer

### Implémentation de commandes supplémentaires
Messages privés (PRIVMSG) : Permettre l'envoi de messages directement à un utilisateur.
PART et QUIT : Gérer la sortie des utilisateurs d'un channel ou du serveur.
(Note : Ce n'est pas demandé par le sujet, mais c'est une proposition de notre IA.)
TOPIC et MODE : Compléter ces commandes pour permettre la gestion du topic et des modes du channel.

### Robustesse
Validation des entrées et gestion des erreurs : Bien que de nombreux tests aient été effectués, il serait intéressant de renforcer la robustesse du code.


Honnêtement, de ce que je vois, on a fait à peu près 70% du taff.
Bien à vous les reufs.
