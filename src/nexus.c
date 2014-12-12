/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**This file is where most of the big stuff is called from. main() is also in here.**/
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "netcore.h"
#include "nexus.h"
#include "config.h"
#include "irc.h"
#include "server.h"

int main(void)
{ ///And as I once cleansed the world with fire, I will destroy you, and your puny project! -Dr. Reed

	//Load NEXUS.conf and whatnot
	printf("Reading configuration... ");
	if (!Config_ReadConfig())
	{
		fprintf(stderr, "Failed to load configuration!\n");
		return 1;
	}
	puts("Done.");
	
	//Connect to the REAL IRC server.
	printf("Connecting to IRC server \"%s:%hu\"... ", IRCConfig.Server, IRCConfig.PortNum);
	if (!IRC_Connect())
	{
		fprintf(stderr, "Unable to connect to IRC server!\n");
		return 1;
	}
	puts("Done.");
	
	
	//Bring up the NEXUS pseudo-IRC-server.
	printf("Bringing up NEXUS server on port %hu... ", NEXUSConfig.PortNum);
	if (!Net_InitServer(NEXUSConfig.PortNum))
	{
		fprintf(stderr, "Failed to bring up NEXUS server on port %hu.\n", NEXUSConfig.PortNum);
		return 1;
	}
	puts("Done.");
	
	while (1)
	{
		IRC_Loop();
		Server_Loop();
	}
	
	return 0;
}

/*Processes data from a client and forwards it to the IRC server.*/
void NEXUS_NEXUS2IRC(const char *Message, struct ClientTree *const Client)
{
	const enum ServerMessageType MsgType = Server_GetMessageType(Message);
	
	switch (MsgType)
	{
		char OutBuf[2048];
		
		default: //Nothing we care about.
		{
			//Append \r\n and send to IRC server.
			snprintf(OutBuf, sizeof OutBuf, "%s\r\n", Message);
			Net_Write(IRCDescriptor, OutBuf, strlen(OutBuf));
			
			break;
		}
		case SERVERMSG_QUIT:
		{
			Server_SendQuit(Client->Descriptor);
			close(Client->Descriptor);
			Server_ClientTree_Del(Client->Descriptor);
			return;
		}
	}
}

/*Processes data from the real IRC server and then sends it to functions that reformat
 * said data so that clients connected to NEXUS IRC won't get confused.*/
void NEXUS_IRC2NEXUS(const char *Message)
{
	const enum IRCMessageType MsgType = IRC_GetMessageType(Message);
	
	switch (MsgType)
	{
		char OutBuf[2048];
		
		default: //Nothing we explicitly have to deal with
		{ //Append a \r\n and then send it to everyone.
			snprintf(OutBuf, sizeof OutBuf, "%s\r\n", Message);
			Server_ForwardToAll(OutBuf);
			
			break;
		}
		case IRCMSG_PART:
		case IRCMSG_JOIN:
		case IRCMSG_NICK:
		{ //We need to make sure the ident and IP match the client's
			char NickTest[64];
			unsigned Inc = 0;
			const char *Test = Message + 1; //Skip past the ':' at the start.
			struct ClientTree *Worker = ClientTreeCore;
			
			for (; Test[Inc] != '!' && Test[Inc] != ' ' && Test[Inc] != '\0' && Inc < sizeof NickTest - 1; ++Inc)
			{ //Copy in the nick for this join/part message.
				NickTest[Inc] = Test[Inc];
			}
			NickTest[Inc] = '\0';
			
			if (!strcmp(NickTest, IRCConfig.Nick)) //This is our nick.
			{ //Since it's our nick, we gotta alter it so that the IP and ident match the client's.
				
				for (; Worker; Worker = Worker->Next)
				{ //Rebuild so it matches their ident and IP.
					if (IRC_AlterMessageOrigin(Message, OutBuf, sizeof OutBuf, Worker))
					{ //If we fail to alter the origin just don't send it.
						strncat(OutBuf, "\r\n", sizeof OutBuf - strlen(OutBuf) - 2);
						Net_Write(Worker->Descriptor, OutBuf, strlen(OutBuf));
					}
				}
				
				break;
				
				if (MsgType == IRCMSG_NICK)
				{ //if it's our nickname, we need to remember that it's been changed.
					char NewNick[sizeof IRCConfig.Nick];
					
					//update the nick.
					IRC_GetMessageData(Message, NewNick);
					strcpy(IRCConfig.Nick, *NewNick == ':' ? NewNick + 1 : NewNick);
				}
			}
			
			//It's not us. Forward verbatim.
			snprintf(OutBuf, sizeof OutBuf, "%s\r\n", Message);
			
			for (; Worker; Worker = Worker->Next)
			{
				Net_Write(Worker->Descriptor, OutBuf, strlen(OutBuf));
			}
			
			break;
		}
		case IRCMSG_PING:
		{ //The IRC server pinged us.
			IRC_Pong(Message);
			break;
		}
			
	}
}


