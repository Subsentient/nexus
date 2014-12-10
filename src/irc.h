/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

///Header for irc.c.

#ifndef __IRC_HEADER__
#define __IRC_HEADER__

#include <stdbool.h>

#define IRC_CODE_OK 1
#define IRC_CODE_NICKTAKEN 433


//Enums
enum IRCMessageType
{
	IRCMSG_INVALID = -1,
	IRCMSG_UNKNOWN,
	IRCMSG_PRIVMSG,
	IRCMSG_NOTICE,
	IRCMSG_MODE,
	IRCMSG_JOIN,
	IRCMSG_PART,
	IRCMSG_PING,
	IRCMSG_PONG,
	IRCMSG_QUIT,
	IRCMSG_KICK,
	IRCMSG_KILL,
	IRCMSG_NICK,
	IRCMSG_INVITE,
	IRCMSG_TOPIC,
	IRCMSG_TOPICORIGIN,
	IRCMSG_NAMES
};

//Functions
bool IRC_GetStatusCode(const char *Message, int *OutNumber);
bool IRC_Connect(void);
void IRC_NickChange(const char *Nick);

#endif //__IRC_HEADER__
