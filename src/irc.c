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

#include "netcore.h"
#include "config.h"
#include "server.h"
#include "irc.h"
#include "nexus.h"

//Functions
void IRC_NickChange(const char *Nick)
{
	char OutBuf[1024];
	
	snprintf(OutBuf, sizeof OutBuf, "NICK %s\r\n", Nick);
	Net_Write(IRCDescriptor, OutBuf, strlen(OutBuf));
}

bool IRC_Connect(void)
{
	char UserString[1024];
	bool ServerConnectOK = false;
	char MessageBuf[2048];
	struct NetReadReturn NRR;
	int IRCCode = 0;
	
	if (!Net_Connect(IRCConfig.Server, IRCConfig.PortNum, &IRCDescriptor))
	{
		fprintf(stderr, "NEXUS has failed to connect to the IRC server %s:%hu\n.", IRCConfig.Server, IRCConfig.PortNum);
		return false;
	}

	//Send USER.
	snprintf(UserString, sizeof UserString, "USER %s 8 * :%s\r\n", IRCConfig.Ident, IRCConfig.RealName);
	Net_Write(IRCDescriptor, UserString, strlen(UserString));
	
	//Send NICK.
	IRC_NickChange(IRCConfig.Nick);
	
	while (!ServerConnectOK)
	{
		//We use nonblocking reads.
		while (1)
		{ ///Read in the response from the server.
			//We're currently assuming the IRC server won't send us half a string at a time with our nonblocking method.
			//I should probably do something about that.
			NRR = Net_Read(IRCDescriptor, MessageBuf, sizeof MessageBuf, true);
#ifdef WIN
			if (NRR.Errno != 0 && NRR.Errno != WSAEWOULDBLOCK)
#else
			if (NRR.Status == 0 || (NRR.Status == -1 && NRR.Errno != EWOULDBLOCK))
#endif
			{
				fprintf(stderr, "NEXUS connected to the IRC server but could not read data from it.\n");
				return false;
			}
#ifdef WIN
			else if (NRR.Errno == WSAEWOULDBLOCK)
#else
			else if (NRR.Status == -1 && NRR.Errno == EWOULDBLOCK)
#endif
			{ //No data, restart loop after tiny break.
#ifdef WIN
				Sleep(1);
#else
				usleep(1000);
#endif //WIN
				continue;
			}
			else break; //We got what we came for.
		}
		
		/**Process the line we got.**/
		
		/*Some servers ping right after you connect. It's horrible, but true.*/
		if (!strncmp("PING ", MessageBuf,  sizeof("PING ") - 1))
		{
			*strchr(MessageBuf, 'I') = 'O';
			Net_Write(IRCDescriptor, MessageBuf, strlen(MessageBuf));
			Net_Write(IRCDescriptor, "\r\n", sizeof "\r\n" - 1);
			continue;
		}
		
		//Get the status code if any.
		IRC_GetStatusCode(MessageBuf, &IRCCode);
		
		switch (IRCCode)
		{
			case IRC_CODE_OK:
				ServerConnectOK = true;
				break;
			case IRC_CODE_NICKTAKEN:
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
		Net_Write(IRCDescriptor, OutBuf, strlen(OutBuf));
	}
	
	return true;
}

	
bool IRC_GetStatusCode(const char *Message, int *OutNumber)
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

enum IRCMessageType IRC_GetMessageType(const char *InStream_)
{ //Another function torn out of aqu4bot.
	const char *InStream = InStream_;
	char Command[32];
	unsigned Inc = 0;
	
	if (InStream[0] != ':') return IRCMSG_INVALID;
	
	if ((InStream = strchr(InStream, ' ')) == NULL) return IRCMSG_INVALID;
	++InStream;
	
	for (; InStream[Inc] != ' '  && InStream[Inc] != '\0' && Inc < sizeof Command - 1; ++Inc)
	{ /*Copy in the command.*/
		Command[Inc] = InStream[Inc];
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
	else if (!strcmp(Command, "353") || !strcmp(Command, "NAMES")) return IRCMSG_NAMES;
	else return IRCMSG_UNKNOWN;
}

bool IRC_Disconnect(void)
{
	char OutBuf[2048];
	
	//I don't care if this fails.
	snprintf(OutBuf, sizeof OutBuf, "QUIT :NEXUS BNC " NEXUS_VERSION " shutting down.\r\n");
	Net_Write(IRCDescriptor, OutBuf, strlen(OutBuf));
	
	return !close(IRCDescriptor);
}

//Where all IRC data processing begins.
void IRC_Loop(void)
{
	struct NetReadReturn NRR;
	char IRCBuf[2048];
	
	//Check IRC for data.
	NRR = Net_Read(IRCDescriptor, IRCBuf, sizeof IRCBuf, true);

#ifdef WIN
	if (NRR.Errno != 0 && NRR.Errno != WSAEWOULDBLOCK)
#else
	if (NRR.Status == 0 || (NRR.Status == -1 && NRR.Errno != EWOULDBLOCK))
#endif
	{ //Error.
		char OutBuf[2048];
		
		IRC_Disconnect();
		
		//Tell everyone what happened.
		snprintf(OutBuf, sizeof OutBuf, ":" CONTROL_NICKNAME "!NEXUS@NEXUS NOTICE %s :NEXUS has lost the connection to %s:%hu and is shutting down.\r\n", IRCConfig.Nick, IRCConfig.Server, IRCConfig.PortNum);		
		Server_ForwardToAll(OutBuf);
		
		Server_SendQuit(-1, "NEXUS has lost the connection to the IRC server."); //Now make them quit.
		Net_ShutdownServer();
		
		exit(1);
	}
#ifdef WIN
	else if (NRR.Errno == WSAEWOULDBLOCK)
#else
	else if (NRR.Status == -1 && NRR.Errno == EWOULDBLOCK)
#endif
	{ //No data from the IRC server
#ifdef WIN
		Sleep(1);
#else
		usleep(1000);
#endif //WIN
		
		return;
	}
	
	//if we get this far, we got data.
	if (!strncmp(IRCBuf, "PING", sizeof "PING" - 1))
	{ //Reply to ping and exit.
		IRC_Pong(IRCBuf);
		return;
	}
	
	//Not a ping, we have real work to do.
	NEXUS_IRC2NEXUS(IRCBuf);
}

void IRC_Pong(const char *Param)
{ //Replies to IRC server's ping request.
	char PingMsg[1024];
	
	snprintf(PingMsg, sizeof PingMsg, "%s\r\n", Param);
	
	*strchr(PingMsg, 'I') = 'O'; //Turn PING to PONG
	
	Net_Write(IRCDescriptor, PingMsg, strlen(PingMsg));
}

bool IRC_GetMessageData(const char *Message, char *OutData)
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


bool IRC_AlterMessageOrigin(const char *InStream, char *OutStream, const unsigned OutStreamSize, const struct ClientList *const Client)
{ //Changes a message's origin to the client's.
	const char *Jump = strchr(InStream, ' ');
	
	if (!Jump) return false;
	
	while (*Jump == ' ') ++Jump; //Skip past the spaces.
	
	snprintf(OutStream, OutStreamSize, ":%s!%s@%s %s", IRCConfig.Nick, Client->Ident, Client->IP, Jump);
	
	return true;
}

bool IRC_BreakdownNick(const char *Message, char *NickOut, char *IdentOut, char *MaskOut)
{
	char ComplexNick[128], *Worker = ComplexNick;
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
		*NickOut = 0;
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
		*NickOut = 0;
		*IdentOut = 0;
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
