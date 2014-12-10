/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**This file is responsible for negotiating with the IRC server.
A lot of this file is *very* similar to aqu4bot's irc.c, and IRC_GetStatusCode() was
directly taken from aqu4bot.
**/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "netcore.h"
#include "config.h"
#include "irc.h"

void IRC_NickChange(const char *Nick)
{
	char OutBuf[1024];
	
	snprintf(OutBuf, sizeof OutBuf, "NICK %s\r\n", Nick);
	Net_Write(IRCDescriptor, OutBuf, strlen(OutBuf));
}

bool IRC_Connect(void)
{ //After connecting to the IRC server, this function
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
			
			if (NRR.Status == 0 || (NRR.Status == -1 && NRR.Errno != EWOULDBLOCK))
			{
				fprintf(stderr, "NEXUS connected to the IRC server but could not read data from it.\n");
				return false;
			}
			else if (NRR.Status == -1 && NRR.Errno == EWOULDBLOCK)
			{ //No data, restart loop after tiny break.
				usleep(1500);
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
				Net_ShutdownServer();
				exit(1);
				break; //Why do I bother
			}
		}
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
