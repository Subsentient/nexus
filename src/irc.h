/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

///Header for irc.c.

#ifndef __IRC_HEADER__
#define __IRC_HEADER__

#include <stdbool.h>
#include "server.h"
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
	IRCMSG_NAMES,
	IRCMSG_WHO
};

//Functions
bool IRC_GetStatusCode(const char *Message, int *OutNumber);
bool IRC_Connect(void);
void IRC_NickChange(const char *Nick);
void IRC_Loop(const char *IRCBuf);
void IRC_Pong(const char *Param);
bool IRC_Disconnect(void);
enum IRCMessageType IRC_GetMessageType(const char *InStream_);
bool IRC_GetMessageData(const char *Message, char *OutData);
bool IRC_AlterMessageOrigin(const char *InStream, char *OutStream, const unsigned OutStreamSize, const struct ClientList *const Client);
bool IRC_BreakdownNick(const char *Message, char *NickOut, char *IdentOut, char *MaskOut);

#endif //__IRC_HEADER__
