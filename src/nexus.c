/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**This file is where most of the big stuff is called from. main() is also in here.**/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "netcore.h"
#include "nexus.h"
#include "config.h"
#include "irc.h"
#include "server.h"
#include "state.h"

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
void NEXUS_NEXUS2IRC(const char *Message, struct ClientList *const Client)
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
		case SERVERMSG_WHO:
		//We are NOT going to give you your stupid who.
			return;
		case SERVERMSG_QUIT:
		{
			Server_SendQuit(Client->Descriptor);
			close(Client->Descriptor);
			Server_ClientList_Del(Client->Descriptor);
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
		case IRCMSG_NAMES: //The server is replying to a names command.
		{
			unsigned Inc = 0;
			const char *Search = strchr(Message, '#');
			char NamesChannel[512];
			char NamesNick[64]; //Used by the do-while loop.
			struct ChannelList *ChannelStruct = NULL;
			
			//Copy in the channel for this.
			for (; Search[Inc] != ' ' && Search[Inc] != '\0' && Inc < sizeof NamesChannel - 1; ++Inc)
			{
				NamesChannel[Inc] = Search[Inc];
			}
			NamesChannel[Inc] = '\0';
			
			//Can't find the channel.
			if (!(ChannelStruct = State_LookupChannel(NamesChannel))) return;
			
			if (Search[Inc] == '\0') return; //Bad data. Failed.
			
			//Skip past all whitespace and the colon.
			Search += Inc;
			while (*Search == ' ' || *Search == ':') ++Search;
			
			//Process each nick in the names list.
			do
			{
				char Symbol = 0; //Are they OP or something?
				
				while (*Search == ' ') ++Search; //Skip past starting whitespace if reentering loop.
				
				//They are an OP or something. Store their symbol.
				if (*Search == '!' || *Search == '@' || *Search == '#'
					|| *Search == '$' || *Search == '%' || *Search == '&'
					|| *Search == '*' || *Search == '+'
					|| *Search == '=' || *Search == '-' || *Search == '('
					|| *Search == ')' || *Search == '?' || *Search == '~')
				{
					Symbol = *Search++;
				}
				
				//Get the nickname for this name.
				for (Inc = 0; Search[Inc] != ' ' && Search[Inc] != '\0' && Inc < sizeof NamesNick - 1; ++Inc)
				{
					NamesNick[Inc] = Search[Inc];
				}
				NamesNick[Inc] = '\0';
				
				
				State_AddUserToChannel(NamesNick, Symbol, ChannelStruct);
				
			} while ((Search = strchr(Search, ' ')));
			
			//Now send the message to the client.
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
			struct ClientList *Worker = ClientListCore;
			
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
				
				switch (MsgType)
				{ //Some specific stuff we need to do
					case IRCMSG_NICK:
					{ //if it's our nickname, we need to remember that it's been changed.
						char NewNick[2048];
						
						//update the nick.
						IRC_GetMessageData(Message, NewNick);
						strcpy(IRCConfig.Nick, *NewNick == ':' ? NewNick + 1 : NewNick);
						break;
					}
					case IRCMSG_JOIN:
					{ //We joined a channel.
						char NewChannel[2048];
						struct ChannelList *Chan;
						IRC_GetMessageData(Message, NewChannel);
						
						Chan = State_AddChannel(NewChannel);
						State_AddUserToChannel(IRCConfig.Nick, 0, Chan);
						break;
					}
					case IRCMSG_PART:
					{ //We left a channel.
						char GoneChannel[2048], *Search;
						
						IRC_GetMessageData(Message, GoneChannel);
						
						//Delete whatever part message they may have placed. We don't care.
						if ((Search = strchr(GoneChannel, ' '))) *Search = '\0';
						
						//Now delete this parted channel.
						State_DelChannel(GoneChannel);
						break;
					}
					default:
						break;
				}
			}
			
			//It's not us. Forward verbatim.
			snprintf(OutBuf, sizeof OutBuf, "%s\r\n", Message);
			
			for (; Worker; Worker = Worker->Next)
			{
				Net_Write(Worker->Descriptor, OutBuf, strlen(OutBuf));
			}
			
			
			switch (MsgType)
			{
				char Nick[64], Ident[64], Mask[256];
				
				case IRCMSG_NICK:
				{ //Change the nick for them in all channels they exist in.
					struct ChannelList *Worker = ChannelListCore;
					char NewNick_[64], *NewNick = NewNick_;
					
					//Get their old nick.
					IRC_BreakdownNick(Message, Nick, Ident, Mask);
					IRC_GetMessageData(Message, NewNick_);
					
					if (*NewNick == ':') ++NewNick;
					
					for (; Worker; Worker = Worker->Next)
					{
						struct UserList *User = State_GetUserInChannel(Nick, Worker);
						char Symbol = 0;
						
						//Doesn't exist in that channel.
						if (!User) continue;
						
						Symbol = User->Symbol; //Back up the symbol, e.g. @ or +.
						State_DelUserFromChannel(Nick, Worker); //Delete the old nickname.
						State_AddUserToChannel(NewNick, Symbol, Worker); //Create the new one.
					}
					break;
				}
				case IRCMSG_PART:
				case IRCMSG_JOIN:
				{ //Add this user to the channel.
					char DaChannel[512], *Search;
					struct ChannelList *ChannelStruct;
					
					IRC_BreakdownNick(Message, Nick, Ident, Mask);
					IRC_GetMessageData(Message, DaChannel);
					
					if ((Search = strchr(DaChannel, ' '))) *Search = '\0';
					
					if (!(ChannelStruct = State_LookupChannel(DaChannel))) break;

					if (MsgType == IRCMSG_JOIN)
					{
						State_AddUserToChannel(Nick, 0, ChannelStruct);
					}
					else
					{
						State_DelUserFromChannel(Nick, ChannelStruct);
					}
					break;
				}
				default:
					break;
			}
			break;
		}
		case IRCMSG_QUIT:
		{ //Handles quitting per-user.
			char Nick[64], Ident[64], Mask[256];
			IRC_BreakdownNick(Message, Nick, Ident, Mask);
			struct ChannelList *Worker = ChannelListCore;
			
			if (!strcmp(Nick, IRCConfig.Nick))
			{ //Uhoh, it's us.
				const char *QuitReason = strchr(Message + 1, ':'); //+ 1 to skip past the FIRST ':'.
				
				if (QuitReason)
				{
					snprintf(OutBuf, sizeof OutBuf,
							":" NEXUS_FAKEHOST " PRIVMSG %s :NEXUS was disconnected from the IRC server. "
							"The reason given was \"%s\". NEXUS is shutting down.\r\n", IRCConfig.Nick, QuitReason + 1);
				}
				else
				{
					snprintf(OutBuf, sizeof OutBuf,
							":" NEXUS_FAKEHOST " PRIVMSG %s :NEXUS was disconnected from the IRC server. "
							"No reason was provided by the server. NEXUS is shutting down.\r\n", IRCConfig.Nick);
				}

					
				Server_ForwardToAll(OutBuf);
				
				IRC_Disconnect(); //Close the IRC server connection.
				Server_SendQuit(-1); //Tell all clients to quit.
				Net_ShutdownServer(); //Bring down the NEXUS server.
				exit(1);
			}
			
			//Yay! Not us!
			for (; Worker; Worker = Worker->Next)
			{
				State_DelUserFromChannel(Nick, Worker); //If it doesn't find it, that's fine.
			}
			
			//Now forward to all our users.
			snprintf(OutBuf, sizeof OutBuf, "%s\r\n", Message);
			Server_ForwardToAll(OutBuf);
		}
	}
}


