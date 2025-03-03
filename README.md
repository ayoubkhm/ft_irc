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

Après la compilation, lancez le serveur avec :

```bash
./ircserv
