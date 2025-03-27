#pragma once
#include <iostream>

void	sendServerRpl(int const client_fd, std::string client_buffer);

# define user_id(nickname, username) (":" + nickname + "!" + username + "@localhost")

# define RPL_WELCOME(user_id, nickname) (":localhost 001 " + nickname + " :Welcome to the Internet Relay Network " + user_id + "\r")
# define RPL_YOURHOST(client, servername, version) (":localhost 002 " + client + " :Your host is " + servername + " (localhost), running version " + version + "\r")
# define RPL_CREATED(client, datetime) (":localhost 003 " + client + " :This server was created " + datetime + "\r")
# define RPL_MYINFO(client, servername, version, user_modes, chan_modes, chan_param_modes) (":localhost 004 " + client + " " + servername + " " + version + " " + user_modes + " " + chan_modes + " " + chan_param_modes + "\r")
//# define RPL_ISUPPORT(client, tokens) (":localhost 005 " + client + " " + tokens " :are supported by this server\r")

# define ERR_UNKNOWNCOMMAND(client, command) (":localhost 421 " + client + " " + command + " :Unknown command\r")
# define ERR_NOTREGISTERED(client) (":localhost 451 " + client + " :You have not registered\r")


// INVITE
# define ERR_NEEDMOREPARAMS(client, command) (":localhost 461 " + client + " " + command + " :Not enough parameters.\r")
# define ERR_NOSUCHCHANNEL(client, channel) (":localhost 403 " + client + " " + channel + " :No such channel\r")
# define ERR_NOSUCHNICK(client, target) ("localhost 401 " + client + " " + target + " :No such nick/channel\r")
# define ERR_ALREADYINVITED(client, target, channel) (":localhost 443 " + client + " " + target + " " + channel + " :is already invited\r")
# define ERR_INVITEYOURSELF(client) (":localhost 502 " + client + " :Cannot invite yourself\r")
# define ERR_USERONCHANNEL(client, nick, channel) (":localhost 443 " + client + " " + nick + " " + channel + " :Is already on channel\r")
# define RPL_INVITE(user_id, invited, channel) (user_id + " INVITE " + invited + " " + channel + "\r")
# define RPL_INVITING(client, target, channel) ("localhost 341 " + client + " inviting " + target + " " + channel + "\r")


// JOIN
# define RPL_JOIN(user_id, channel) (user_id + " JOIN :" +  channel + "\r")
//# define ERR_BANNEDFROMCHAN(client, channel) ("474 " + client + " #" + channel + " :Cannot join channel (+b)\r")
# define ERR_BADCHANNELKEY(client, channel) ("localhost 475 " + client + " " + channel + " :Cannot join channel (+k)\r")
# define ERR_ALREADYJOINED(client, channel) (":localhost 443 " + client + " " + channel + " :Is already on channel\r")
# define ERR_INVITEONLY(client, channel) (":localhost 473 " + client + " " + channel + " :Cannot join channel (+i)\r")


// KICK
# define ERR_USERNOTINCHANNEL(client, nickname, channel) ("441 " + client + " " + nickname + " " + channel + " :They aren't on that channel\r")
// # define ERR_CHANOPRIVSNEEDED(client, channel) ("482 " + client + " #" +  channel + " :You're not channel operator\r")
# define RPL_KICK(channel, kicked, reason) ("localhost KICK " + channel + " " + kicked + " " + reason + "\r")
# define ERR_CANNOTKICKSELF(client) ("502 " + client + " :Cannot kick yourself\r")


// KILL
/* # define ERR_NOPRIVILEGES(client) ("481 " + client + " :Permission Denied- You're not an IRC operator\r")
# define RPL_KILL(user_id, killed, comment) (user_id + " KILL " + killed + " " + comment + "\r") */

// MODE
/* user mode */
#define MODE_USERMSG(client, mode) (":" + client + " MODE " + client + " :" + mode + "\r")
#define ERR_UMODEUNKNOWNFLAG(client) (":localhost 501 " + client + " :Unknown MODE flag\r")
#define ERR_USERSDONTMATCH(client) ("localhost 502 " + client + " :Cant change mode for other users\r")
#define RPL_UMODEIS(client, mode) (":localhost 221 " + client + " " + mode + "\r")
#define ERR_CANNOTREMOVEOP(client, channel) (":localhost 461 " + client + " " + channel + " :Operator can't be removed\r")

/* channel mode */
#define MODE_CHANNELMSG(channel, mode) (":localhost MODE " + channel + " " + mode + "\r")
#define MODE_CHANNELMSGWITHPARAM(channel, mode, param) (":localhost MODE " + channel + " " + mode + " " + param + "\r")
#define RPL_CHANNELMODEIS(client, channel, mode) (":localhost 324 " + client + " " + channel + " " + mode + "\r")
#define RPL_CHANNELMODEISWITHKEY(client, channel, mode, password) (":localhost 324 " + client + " " + channel + " " + mode + " " + password + "\r")
#define ERR_CANNOTSENDTOCHAN(client, channel) ("localhost 404 " + client + " " + channel + " :Cannot send to channel\r")
#define ERR_CHANNELISFULL(client, channel) ("localhost 471 " + client + " " + channel + " :Cannot join channel (+l)\r")
#define ERR_CHANOPRIVSNEEDED(client, channel) (":localhost 482 " + client + " " + channel + " :You're not channel operator\r")
#define ERR_INVALIDMODEPARAM(client, channel, mode, password) ("localhost 696 " + client + " " + channel + " " + mode + " " + password + " : password must only contained alphabetic character\r")
// RPL_ERR a broadcoast quand user pas +v ou operator veut parler
      // dans notre cas c'était tiff (client) qui voulait send a message
      // :lair.nl.eu.dal.net 404 tiff #pop :Cannot send to channel
#define RPL_ADDVOICE(nickname, username, channel, mode, param) (":" + nickname + "!" + username + "@localhost MODE " + channel + " " + mode + " " + param + "\r")

/* // MOTD
#define ERR_NOSUCHSERVER(client, servername) (":localhost 402 " + client + " " + servername + " :No such server\r")
#define ERR_NOMOTD(client) (":localhost 422 " + client + " :MOTD File is missing\r")
#define RPL_MOTDSTART(client, servername) (":localhost 375 " + client + " :- " + servername + " Message of the day - \r")
#define RPL_MOTD(client, motd_line) (":localhost 372 " + client + " :" + motd_line + "\r")
#define RPL_ENDOFMOTD(client) (":localhost 376 " + client + " :End of /MOTD command.\r")


// NAMES
# define RPL_NAMREPLY(client, symbol, channel, list_of_nicks) (":localhost 353 " + client + " " + symbol + " " + channel + " :" + list_of_nicks + "\r")
# define RPL_ENDOFNAMES(client, channel) (":localhost 366 " + client + " " + channel + " :End of /NAMES list.\r") */

// NICK
# define ERR_NONICKNAMEGIVEN(client) (":localhost 431 " + client + " :There is no nickname.\r")
# define ERR_ERRONEUSNICKNAME(client, nickname) (":localhost 432 " + client + " " + nickname + " :Erroneus nickname\r")
# define ERR_NICKNAMEINUSE(client, nickname) (":localhost 433 " + client + " " + nickname + " :Nickname is already in use.\r")
# define RPL_NICK(oclient, uclient, client) (":" + oclient + "!" + uclient + "@localhost NICK " +  client + "\r")

// NOTICE
/* # define RPL_NOTICE(nick, username, target, message) (":" + nick + "!" + username + "@localhost NOTICE " + target + " " + message + "\r") */

// OPER
/* # define ERR_NOOPERHOST(client) ("491 " + client + " :No O-lines for your host\r")
# define RPL_YOUREOPER(client) ("381 " + client + " :You are now an IRC operator\r") */

// PART
# define RPL_PART(user_id, channel, reason) (std::string(user_id) + " PART " + channel + " " + (std::string(reason).empty() ? "." : std::string(reason)) + "\r")
# define ERR_NOSUCHCHANNEL(client, channel) (":localhost 403 " + client + " " + channel + " :No such channel\r")
# define ERR_NOTONCHANNEL(client, channel) (":localhost 442 " + client + " " + channel + " :You're not on that channel\r")


// PASS
# define ERR_PASSWDMISMATCH(client) (":localhost 464 " + client + " :Password incorrect.\r")

// PING
# define RPL_PONG(user_id, token) (user_id + " PONG " + token + "\r")

// QUIT
/* # define RPL_QUIT(user_id, reason) (user_id + " QUIT :Quit: " + reason + "\r")
# define RPL_ERROR(user_id, reason) (user_id + " ERROR :" + reason + "\r") */

// PRIVMSG
# define ERR_NOSUCHNICK(client, target) ("localhost 401 " + client + " " + target + " :No such nick/channel\r")
# define ERR_NORECIPIENT(client) ("localhost 411 " + client + " :No recipient given PRIVMSG\r")
# define ERR_NOTEXTTOSEND(client) ("localhost 412 " + client + " :No text to send\r")
# define RPL_PRIVMSG(nick, username, target, message) (":" + nick + "!" + username + "@localhost PRIVMSG " + target + " " + message + "\r")

// TOPIC
# define RPL_TOPIC(client, channel, topic) (":localhost 332 " + client + " " + channel + " " + topic + "\r")
# define RPL_NOTOPIC(client, channel) (":localhost 331 " + client + " " + channel + " :No topic is set\r")

// USER
# define ERR_ALREADYREGISTERED(client) (":localhost 462 " + client + " :You may not reregister.\r")