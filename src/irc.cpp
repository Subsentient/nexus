/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**This file is responsible for negotiating with the real IRC server.
A lot of this file is *very* similar to aqu4bot's irc.c, and some functions were
just torn directly out of aqu4bot.
**/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#ifdef WIN
#include <winsock2.h>
#endif

#include "substrings/substrings.h"
#include "netcore.h"
#include "config.h"
#include "server.h"
#include "irc.h"
#include "nexus.h"

//Functions
void IRC::NickChange(const char *Nick)
{
	char OutBuf[1024];
	
	snprintf(OutBuf, sizeof OutBuf, "NICK %s\r\n", Nick);
	Net::Write(IRCDescriptor, OutBuf, strlen(OutBuf));
}

bool IRC::Connect(void)
{
	char UserString[1024];
	bool ServerConnectOK = false;
	char MessageBuf[2048];
	int IRCCode = 0;
	
	if (!Net::Connect(IRCConfig.Server, IRCConfig.PortNum, &IRCDescriptor))
	{
		fprintf(stderr, "NEXUS has failed to connect to the IRC server %s:%hu\n.", IRCConfig.Server, IRCConfig.PortNum);
		return false;
	}

	//Send USER.
	snprintf(UserString, sizeof UserString, "USER %s 8 * :%s\r\n", IRCConfig.Ident, IRCConfig.RealName);
	Net::Write(IRCDescriptor, UserString, strlen(UserString));
	
	//Send NICK.
	IRC::NickChange(IRCConfig.Nick);
	
	while (!ServerConnectOK)
	{
		const bool NRR = Net::Read(IRCDescriptor, MessageBuf, sizeof MessageBuf, true);
		
		if (!NRR)
		{
			fputs("Failed to connect to IRC server.\n", stderr);
			return false;
		}
		
		/**Process the line we got.**/
		
		/*Some servers ping right after you connect. It's horrible, but true.*/
		if (!strncmp("PING ", MessageBuf,  sizeof("PING ") - 1))
		{
			*strchr(MessageBuf, 'I') = 'O';
			Net::Write(IRCDescriptor, MessageBuf, strlen(MessageBuf));
			Net::Write(IRCDescriptor, "\r\n", sizeof "\r\n" - 1);
			continue;
		}
		
		//Get the status code if any.
		IRC::GetStatusCode(MessageBuf, &IRCCode);
		
		switch (IRCCode)
		{
			case IRCCODE_OK:
				ServerConnectOK = true;
				break;
			case IRCCODE_NICKTAKEN:
			{ //This presents a problem.
				fprintf(stderr, "Nickname for IRC server is in use.\n");
				return false;
				break; //Why do I bother
			}
		}
	}
	
	//Send NickServ password.
	if (*IRCConfig.NickServPassword)
	{
		char OutBuf[2048];
		
		snprintf(OutBuf, sizeof OutBuf, "PRIVMSG NickServ :identify %s %s\r\n", IRCConfig.NickServUser, IRCConfig.NickServPassword);
		Net::Write(IRCDescriptor, OutBuf, strlen(OutBuf));
	}
	
	return true;
}

	
bool IRC::GetStatusCode(const char *Message, int *OutNumber)
{ /*Returns true if we get a status code.*/
	unsigned Inc = 0; //This function ripped directly out of aqu4bot.
	char Num[64];
	
	if (!(Message = strchr(Message, ' '))) return false;
	++Message;
	
	for (; Message[Inc] != ' ' && Message[Inc] != '\0' && Inc < sizeof Num - 1; ++Inc)
	{
		if (!isdigit(Message[Inc])) return false;
		Num[Inc] = Message[Inc];
	}
	Num[Inc] = '\0';
	
	*OutNumber = atoi(Num);
	return true;
}

enum IRCMessageType IRC::GetMessageType(const char *InStream_)
{ //Another function torn out of aqu4bot.
	const char *InStream = InStream_;
	char Command[32];
	unsigned Inc = 0;
	
	if (InStream[0] != ':') return IRCMSG_INVALID;
	
	if ((InStream = strchr(InStream, ' ')) == NULL) return IRCMSG_INVALID;
	++InStream;
	
	for (; InStream[Inc] != ' '  && InStream[Inc] != '\0' && Inc < sizeof Command - 1; ++Inc)
	{ /*Copy in the command.*/
		Command[Inc] = toupper(InStream[Inc]);
	}
	Command[Inc] = '\0';
	
	/*Time for the comparison.*/
	if (!strcmp(Command, "PRIVMSG")) return IRCMSG_PRIVMSG;
	else if (!strcmp(Command, "NOTICE")) return IRCMSG_NOTICE;
	else if (!strcmp(Command, "MODE")) return IRCMSG_MODE;
	else if (!strcmp(Command, "JOIN")) return IRCMSG_JOIN;
	else if (!strcmp(Command, "PART")) return IRCMSG_PART;
	else if (!strcmp(Command, "PING")) return IRCMSG_PING;
	else if (!strcmp(Command, "PONG")) return IRCMSG_PONG;
	else if (!strcmp(Command, "NICK")) return IRCMSG_NICK;
	else if (!strcmp(Command, "QUIT")) return IRCMSG_QUIT;
	else if (!strcmp(Command, "KICK")) return IRCMSG_KICK;
	else if (!strcmp(Command, "KILL")) return IRCMSG_KILL;
	else if (!strcmp(Command, "INVITE")) return IRCMSG_INVITE;
	else if (!strcmp(Command, "332") || !strcmp(Command, "TOPIC")) return IRCMSG_TOPIC;
	else if (!strcmp(Command, "333")) return IRCMSG_TOPICORIGIN;
	else if (!strcmp(Command, "352")) return IRCMSG_WHO;
	else if (!strcmp(Command, "353") || !strcmp(Command, "NAMES")) return IRCMSG_NAMES;
	else if (!strcmp(Command, "324")) return IRCMSG_CHANMODE1;
	else if (!strcmp(Command, "329")) return IRCMSG_CHANMODE2;
	else return IRCMSG_UNKNOWN;
}

bool IRC::Disconnect(void)
{
	char OutBuf[2048];
	
	//I don't care if this fails.
	snprintf(OutBuf, sizeof OutBuf, "QUIT :NEXUS BNC " NEXUS_VERSION " shutting down.\r\n");
	Net::Write(IRCDescriptor, OutBuf, strlen(OutBuf));
	
	return Net::Close(IRCDescriptor);
}

//Where all IRC data processing begins.
void IRC::Loop(const char *IRCBuf)
{
	//if we get this far, we got data.
	if (!strncmp(IRCBuf, "PING", sizeof "PING" - 1))
	{ //Reply to ping and exit.
		IRC::Pong(IRCBuf);
		return;
	}
	
	//Not a ping, we have real work to do.
	NEXUS::IRC2NEXUS(IRCBuf);
}

void IRC::Pong(const char *Param)
{ //Replies to IRC server's ping request.
	char PingMsg[1024];
	
	SubStrings.Copy(PingMsg, Param, sizeof PingMsg);
	SubStrings.Cat(PingMsg, "\r\n", sizeof PingMsg);
	
	*strchr(PingMsg, 'I') = 'O'; //Turn PING to PONG
	
	Net::Write(IRCDescriptor, PingMsg, strlen(PingMsg));
}

bool IRC::GetMessageData(const char *Message, char *OutData)
{
	const char *Worker = Message;
	
	if (!(Worker = strchr(Worker, ' '))) return false;
	++Worker;
	
	if (!(Worker = strchr(Worker, ' '))) return false;
	++Worker;
	
	if (*Worker == ':') ++Worker; /*This might cause problems somewhere, but I hope not. Saves work fiddling with server specific stuff.*/
		
	strcpy(OutData, Worker);
	return true;
}


bool IRC::AlterMessageOrigin(const char *InStream, char *OutStream, const unsigned OutStreamSize, const struct ClientListStruct *const Client)
{ //Changes a message's origin to the client's.
	const char *Jump = strchr(InStream, ' ');
	
	if (!Jump) return false;
	
	while (*Jump == ' ') ++Jump; //Skip past the spaces.
	
	snprintf(OutStream, OutStreamSize, ":%s!%s@%s %s", IRCConfig.Nick, Client->Ident, Client->IP, Jump);
	
	return true;
}

bool IRC::BreakdownNick(const char *Message, char *NickOut, char *IdentOut, char *MaskOut)
{
	char ComplexNick[256], *Worker = ComplexNick;
	unsigned Inc = 0;
	
	for (; Message[Inc] != ' ' && Message[Inc] != '\0'; ++Inc)
	{
		ComplexNick[Inc] = Message[Inc];
	}
	ComplexNick[Inc] = '\0';
	
	if (*Worker == ':') ++Worker;
	
	for (Inc = 0; Worker[Inc] != '!' &&  Worker[Inc] != ' ' && Worker[Inc] != '\0'; ++Inc)
	{
		NickOut[Inc] = Worker[Inc];
	}
	NickOut[Inc] = '\0';
	
	if (Worker[Inc] != '!')
	{
		*IdentOut = 0;
		*MaskOut = 0;
		return false;
	}
	
	Worker += Inc + 1;
	
	if (*Worker == '~') ++Worker;
	
	for (Inc = 0; Worker[Inc] != '@' && Worker[Inc] != ' ' && Worker[Inc] != '\0'; ++Inc)
	{
		IdentOut[Inc] = Worker[Inc];
	}
	IdentOut[Inc] = '\0';
	
	if (Worker[Inc] != '@')
	{
		*MaskOut = 0;
		return false;
	}
	
	Worker += Inc + 1;
	
	for (Inc = 0; Worker[Inc] != ' ' && Worker[Inc] != '\0'; ++Inc)
	{
		MaskOut[Inc] = Worker[Inc];
	}
	MaskOut[Inc] = '\0';
	
	return true;
}
