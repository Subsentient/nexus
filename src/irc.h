/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

///Header for irc.c.

#ifndef __IRC_HEADER__
#define __IRC_HEADER__

#include <stdbool.h>
#include "server.h"
#define IRCCODE_OK 1
#define IRCCODE_NICKTAKEN 433

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
	IRCMSG_WHO,
	IRCMSG_CHANMODE1,
	IRCMSG_CHANMODE2
};

//Functions
namespace IRC
{
	bool GetStatusCode(const char *Message, int *OutNumber);
	bool Connect(void);
	void NickChange(const char *Nick);
	void Loop(const char *IRCBuf);
	void Pong(const char *Param);
	bool Disconnect(void);
	enum IRCMessageType GetMessageType(const char *InStream_);
	bool GetMessageData(const char *Message, char *OutData);
	bool AlterMessageOrigin(const char *InStream, char *OutStream, const unsigned OutStreamSize, const struct ClientListStruct *const Client);
	bool BreakdownNick(const char *Message, char *NickOut, char *IdentOut, char *MaskOut);
}
#endif //__IRC_HEADER__
