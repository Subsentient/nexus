/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#ifdef WIN
#include <winsock2.h>
#endif 

#include "server.h"
#include "netcore.h"
#include "config.h"
#include "nexus.h"
#include "state.h"
#include "scrollback.h"

#include "substrings/substrings.h"

/**This file has our IRC pseudo-server that we run ourselves and its interaction with clients.**/

//Globals
struct ClientList *ClientListCore;
struct ClientList *CurrentClient, *PreviousClient;

//Prototypes
static void Server_SendChannelNamesList(const struct ChannelList *const Channel, const int ClientDescriptor);
static void Server_SendChannelRejoin(const struct ChannelList *const Channel, const int ClientDescriptor);

//Functions

struct ClientList *Server_ClientList_Lookup(const int Descriptor)
{
	struct ClientList *Worker = ClientListCore;
	
	for (; Worker; Worker = Worker->Next)
	{
		if (Worker->Descriptor == Descriptor)
		{
			return Worker;
		}
	}
	
	return NULL;
}

struct ClientList *Server_ClientList_Add(const struct ClientList *const InStruct)
{
	struct ClientList *Worker = ClientListCore, *TempNext, *TempPrev;
	
	if (!ClientListCore)
	{
		Worker = ClientListCore = calloc(1, sizeof(struct ClientList)); //use calloc to zero it out
	}
	else
	{
		while (Worker->Next) Worker = Worker->Next;
		Worker->Next = calloc(1, sizeof(struct ClientList));
		Worker->Next->Prev = Worker;
		Worker = Worker->Next;
	}
	
	TempNext = Worker->Next;
	TempPrev = Worker->Prev;
	
	*Worker = *InStruct;
	
	Worker->Next = TempNext;
	Worker->Prev = TempPrev;
	
	return Worker;
}


void Server_ClientList_Shutdown(void)
{
	struct ClientList *Worker = ClientListCore, *Next;

	for (; Worker; Worker = Next)
	{
		Next = Worker->Next;
		free(Worker);
	}
	
	ClientListCore = NULL;
	PreviousClient = NULL;
	CurrentClient = NULL;
}

bool Server_ClientList_Del(const int Descriptor)
{
	struct ClientList *Worker = ClientListCore;
	
	if (!ClientListCore) return false;
	
	for (; Worker; Worker = Worker->Next)
	{
		if (Worker->Descriptor == Descriptor)
		{ //Match.
			
			if (Worker == PreviousClient)
			{
				PreviousClient = (struct ClientList*)-1; //We do -1 so it still tests as unequal for any new comparisons.
			}
			if (Worker == CurrentClient)
			{
				CurrentClient = (struct ClientList*)-1;
			}
			
			if (Worker == ClientListCore)
			{
				if (Worker->Next)
				{ //We're the first one but there are others ahead of us.
					ClientListCore = Worker->Next;
					ClientListCore->Prev = NULL;
					free(Worker);
				}
				else
				{ //Just us.
					free(Worker);
					ClientListCore = NULL;
				}
			}
			else
			{
				Worker->Prev->Next = Worker->Next;
				if (Worker->Next) Worker->Next->Prev = Worker->Prev;
				free(Worker);
			}
			
			return true;
		}
	}
	
	return false;
}

bool Server_ForwardToAll(const char *const InStream)
{ //This function sends the provided text stream to all clients. Very simple.
	struct ClientList *Worker = ClientListCore;
	
	if (!Worker) return false;
	
	for (; Worker; Worker = Worker->Next)
	{
		Net_Write(Worker->Descriptor, InStream, strlen(InStream));
	}
	return true;
}

void Server_SendQuit(const int Descriptor, const char *const Reason)
{ //Tells all clients or just one client to quit
	struct ClientList *Worker = ClientListCore;
	char OutBuf[2048];
	
	for (; Worker; Worker = Worker->Next)
	{
		//If not on "everyone" mode we check if the descriptor matches.
		if (Descriptor != -1 && Descriptor != Worker->Descriptor) continue; 
		
		if (Reason)
		{
			snprintf(OutBuf, sizeof OutBuf, ":%s!%s@%s QUIT :%s\r\n", IRCConfig.Nick, Worker->Ident, Worker->IP, Reason);
		}
		else
		{
			snprintf(OutBuf, sizeof OutBuf, ":%s!%s@%s QUIT :Disconnected from NEXUS.\r\n", IRCConfig.Nick, Worker->Ident, Worker->IP);
		}
		Net_Write(Worker->Descriptor, OutBuf, strlen(OutBuf));
	}
}

void Server_SendIRCWelcome(const int ClientDescriptor)
{
	char OutBuf[2048];
	struct ClientList *Client = Server_ClientList_Lookup(ClientDescriptor), *CWorker = NULL;
	struct ChannelList *Worker = ChannelListCore;
	int ClientCount = 0;
	
	if (!Client) return;
	
	//First thing we send is our greeting, telling them they're connected OK.
	snprintf(OutBuf, sizeof OutBuf, ":" NEXUS_FAKEHOST " 001 %s :NEXUS is forwarding you to server %s:%hu\r\n",
			IRCConfig.Nick, IRCConfig.Server, IRCConfig.PortNum); //Putting IRCConfig.Nick here is the same as sending a NICK command.
	Net_Write(Client->Descriptor, OutBuf, strlen(OutBuf));
	
	//Tell them to join all channels we are already in.
	for (; Worker; Worker = Worker->Next)
	{
		Server_SendChannelRejoin(Worker, Client->Descriptor);
	}
	
	
	//Count clients for our next cool little trick.
	for (CWorker = ClientListCore; CWorker; CWorker = CWorker->Next) ++ClientCount;
	
	/**Send a MOTD.**/
	
	//Send the beginning.
	snprintf(OutBuf, sizeof OutBuf, ":" NEXUS_FAKEHOST " 375 %s :Welcome to NEXUS " NEXUS_VERSION
			". You are being forwarded to \"%s:%hu\".\r\n", IRCConfig.Nick, IRCConfig.Server, IRCConfig.PortNum);
	Net_Write(Client->Descriptor, OutBuf, strlen(OutBuf));
	
	//For dumb bots that connect, make it abundantly clear what their nick should be.
	snprintf(OutBuf, sizeof OutBuf, ":%s!%s@%s NICK :%s\r\n", Client->OriginalNick, Client->Ident, Client->IP, IRCConfig.Nick);
	Net_Write(Client->Descriptor, OutBuf, strlen(OutBuf));
	
	//Send the middle.
	snprintf(OutBuf, sizeof OutBuf,
			":" NEXUS_FAKEHOST " 372 %s :There are currently %d other instances connected to this NEXUS server.\r\n",
			IRCConfig.Nick, ClientCount - 1);
	Net_Write(Client->Descriptor, OutBuf, strlen(OutBuf));
	
	//Send the end.
	snprintf(OutBuf, sizeof OutBuf, ":" NEXUS_FAKEHOST " 376 %s :End of MOTD.\r\n", IRCConfig.Nick);
	Net_Write(Client->Descriptor, OutBuf, strlen(OutBuf));
	
	//Send all scrollback.
	struct ScrollbackList *SWorker = ScrollbackCore;
	char ScrollBuf[2048], SelfOrigin[256];
	
	snprintf(SelfOrigin, sizeof SelfOrigin, "%s!%s@%s", IRCConfig.Nick, Client->Ident, Client->IP);
	
	for (; SWorker; SWorker = SWorker->Next)
	{ //Loop through sending the scrollback.
		struct tm *TimeStruct = localtime(&SWorker->Time);
		char TimeBuf[128];
		const char *Origin = SWorker->Origin;
		
		strftime(TimeBuf, sizeof TimeBuf, "%H:%M %Y-%m-%d", TimeStruct);

		if (!Origin)
		{ //We want to use ourselves as the origin if none specified.
			Origin = SelfOrigin;
		}
		
		if (SubStrings.StartsWith("\1ACTION ", SWorker->Msg))
		{
			char *NewMsg = calloc(strlen(SWorker->Msg) + 1, 1);
			
			//Don't bother trying to understand the arithmetic here.
			SubStrings.Copy(NewMsg, SWorker->Msg + (sizeof "\1ACTION " - 1), (strlen(SWorker->Msg) + 1) - (sizeof "\1ACTION " - 1));
			
			//Chop off the \x01 at the end.
			SubStrings.StripTrailingChars(NewMsg, "\001");
			
			//Build the message.
			snprintf(ScrollBuf, sizeof ScrollBuf, ":%s PRIVMSG %s :\0034[%s]\3 ** %s %s **\r\n",
					Origin, (SWorker->Target ? SWorker->Target : IRCConfig.Nick), TimeBuf, Origin, NewMsg);
			free(NewMsg);
		}
		else
		{
			snprintf(ScrollBuf, sizeof ScrollBuf, ":%s PRIVMSG %s :\0034[%s]\3 %s\r\n",
				Origin, (SWorker->Target ? SWorker->Target : IRCConfig.Nick), TimeBuf, SWorker->Msg);
		}
		
		//Now send it to the client.
		Net_Write(Client->Descriptor, ScrollBuf, strlen(ScrollBuf));
	}
	
}

static void Server_SendChannelNamesList(const struct ChannelList *const Channel, const int ClientDescriptor)
{
	char OutBuf[2048];
	unsigned Ticker = 1;
	struct UserList *Worker = Channel->UserList;
	
	//Make basic formatting.
SendBegin:
	snprintf(OutBuf, sizeof OutBuf, ":" NEXUS_FAKEHOST " 353 %s * %s :", IRCConfig.Nick, Channel->Channel);
	
	for (Ticker = 1; Worker; Worker = Worker->Next, ++Ticker)
	{
		unsigned OutBufLen = strlen(OutBuf);
		char Sym[2] = { '\0' };
		
		//Reconstitute the mode flag for this user.
		*Sym = State_UserModes_Get_Mode2Symbol(Worker->Modes);
		
		snprintf(OutBuf + OutBufLen, sizeof OutBuf - OutBufLen, "%s%s ", Sym, Worker->Nick);
		
		if (Worker->Next == NULL|| Ticker == 20) //We pick 20 because sizeof OutBuf (2048) / sizeof Worker->Nick (64) == 32
		{

			
			OutBuf[strlen(OutBuf) - 1] = '\0';
			strcat(OutBuf, "\r\n");
			Net_Write(ClientDescriptor, OutBuf, strlen(OutBuf));
			
			if (Worker->Next)
			{
				Worker = Worker->Next; //So we don't loop on the same nick.
				goto SendBegin;
			}
		}
	}			
	
	snprintf(OutBuf, sizeof OutBuf, ":" NEXUS_FAKEHOST " 366 %s %s :End of /NAMES list.\r\n", IRCConfig.Nick, Channel->Channel);
	Net_Write(ClientDescriptor, OutBuf, strlen(OutBuf));
	
}

static void Server_SendChannelRejoin(const struct ChannelList *const Channel, const int ClientDescriptor)
{ //Sends the list of channels we are in to the client specified.
	char OutBuf[2048];
	struct ClientList *Client = Server_ClientList_Lookup(ClientDescriptor);
	
	if (!Client) return;
	
	//Send the join command.
	snprintf(OutBuf, sizeof OutBuf, ":%s!%s@%s JOIN %s\r\n", IRCConfig.Nick, Client->Ident, Client->IP, Channel->Channel);
	Net_Write(ClientDescriptor, OutBuf, strlen(OutBuf));
	
	//Send the topic and the setter of the topic.
	if (*Channel->Topic && *Channel->WhoSetTopic && Channel->WhenSetTopic != 0)
	{
		snprintf(OutBuf, sizeof OutBuf, ":" NEXUS_FAKEHOST " 332 %s %s :%s\r\n", IRCConfig.Nick, Channel->Channel, Channel->Topic);
		Net_Write(ClientDescriptor, OutBuf, strlen(OutBuf));
		
		snprintf(OutBuf, sizeof OutBuf, ":" NEXUS_FAKEHOST " 333 %s %s %s %u\r\n", IRCConfig.Nick, Channel->Channel, Channel->WhoSetTopic, Channel->WhenSetTopic);
		Net_Write(ClientDescriptor, OutBuf, strlen(OutBuf));
	}
	
	//Send list of users.
	Server_SendChannelNamesList(Channel, ClientDescriptor);
}

struct ClientList *Server_AcceptLoop(void)
{
	struct ClientList TempClient;
	struct NetReadReturn NRR;
	char InBuf[2048];
	struct ClientList *Client = NULL;
	bool UserProvided = false, NickProvided = false;
	bool PasswordProvided = false;
	
	if (!Net_AcceptClient(&TempClient.Descriptor, TempClient.IP, sizeof TempClient.IP))
	{ //No client.
		return NULL;
	}


	///Apparently there is a client.
	Client = Server_ClientList_Add(&TempClient); //Store their information.

	/**Continuously try to read their replies until we get them.**/
	while (!UserProvided || !NickProvided)
	{ //Wait for their greeting.
		while (1)
		{
			NRR = Net_Read(Client->Descriptor, InBuf, sizeof InBuf, true);

#ifdef WIN
			if (NRR.Errno != 0 && NRR.Errno != WSAEWOULDBLOCK)
#else
			if (NRR.Status == 0 || (NRR.Status == -1 && NRR.Errno != EWOULDBLOCK))
#endif
			{ //We lost them.
				close(Client->Descriptor); //Close their connection.
				Server_ClientList_Del(Client->Descriptor); //Delete their record.
				return NULL;
			}
#ifdef WIN
			else if (NRR.Errno == WSAEWOULDBLOCK)
#else
			else if (NRR.Status == -1 && NRR.Errno == EWOULDBLOCK)
#endif
			{ //They just didn't reply to us yet.
				continue;
#ifdef WIN
				Sleep(1);
#else
				usleep(1000);
#endif
			}
			else break;
		}
		
		//Does it start with USER?
		if (!strncmp(InBuf, "USER", sizeof "USER" - 1) || !strncmp(InBuf, "user", sizeof "user" - 1))
		{ //This information is needed to fool the IRC clients.
			const char *TWorker = InBuf + sizeof "USER";
			unsigned Inc = 0;
			
			//If we want a password, WE WANT A PASSWORD. You're supposed to send PASS first, dunce!
			if (*NEXUSConfig.ServerPassword && !PasswordProvided)
			{
				close(Client->Descriptor);
				return NULL;
			}
			
			while (*TWorker == ' ' || *TWorker == ':') ++TWorker;
			
			for (; TWorker[Inc] != ' ' && TWorker[Inc] != '\0' && Inc < sizeof Client->Ident - 1; ++Inc)
			{ //Copy in the ident they sent us.
				Client->Ident[Inc] = TWorker[Inc];
			}
			Client->Ident[Inc] = '\0';
		
			UserProvided = true;
			
		}
		else if (!strncmp(InBuf, "NICK", sizeof "NICK" - 1) || !strncmp(InBuf, "nick", sizeof "nick" - 1))
		{
			const char *TWorker = InBuf + sizeof "nick";
			
			//If we want a password, WE WANT A PASSWORD.
			if (*NEXUSConfig.ServerPassword && !PasswordProvided)
			{
				close(Client->Descriptor);
				return NULL;
			}
			
			while (*TWorker == ' ' || *TWorker == ':') ++TWorker;
				
			strncpy(Client->OriginalNick, TWorker, sizeof Client->OriginalNick - 1); //Copy in their chosen nick.
			Client->OriginalNick[sizeof Client->OriginalNick - 1] = '\0';
			
			NickProvided = true;
			
		}
		else if (!strncmp(InBuf, "PASS", sizeof "PASS" - 1))
		{
			const char *TW = InBuf + sizeof "PASS";
			
			if (!*NEXUSConfig.ServerPassword)
			{ //We don't NEED a password. Just ignore this.
				continue;
			}
				
			while (*TW == ' ') ++TW;
			
			if (!strcmp(TW, NEXUSConfig.ServerPassword)) PasswordProvided = true;
			else
			{ //Wrong password.
				close(Client->Descriptor);
				return NULL;
			}
		}
		continue;
	}
	
	//Time to welcome them.
	Server_SendIRCWelcome(Client->Descriptor);
	
	//Return the client we found.
	return Client;
}


enum ServerMessageType Server_GetMessageType(const char *InStream_)
{ //Another function torn out of aqu4bot.
	const char *InStream = InStream_;
	char Command[32];
	unsigned Inc = 0;
	
	for (; InStream[Inc] != ' '  && InStream[Inc] != '\0' && Inc < sizeof Command - 1; ++Inc)
	{ /*Copy in the command.*/
		Command[Inc] = toupper(InStream[Inc]);
	}
	Command[Inc] = '\0';
	
	/*Time for the comparison.*/
	if (!strcmp(Command, "PRIVMSG")) return SERVERMSG_PRIVMSG;
	else if (!strcmp(Command, "NOTICE")) return SERVERMSG_NOTICE;
	else if (!strcmp(Command, "MODE")) return SERVERMSG_MODE;
	else if (!strcmp(Command, "JOIN")) return SERVERMSG_JOIN;
	else if (!strcmp(Command, "PART")) return SERVERMSG_PART;
	else if (!strcmp(Command, "PING")) return SERVERMSG_PING;
	else if (!strcmp(Command, "PONG")) return SERVERMSG_PONG;
	else if (!strcmp(Command, "NICK")) return SERVERMSG_NICK;
	else if (!strcmp(Command, "QUIT")) return SERVERMSG_QUIT;
	else if (!strcmp(Command, "KICK")) return SERVERMSG_KICK;
	else if (!strcmp(Command, "KILL")) return SERVERMSG_KILL;
	else if (!strcmp(Command, "INVITE")) return SERVERMSG_INVITE;
	else if (!strcmp(Command, "TOPIC")) return SERVERMSG_TOPIC;
	else if (!strcmp(Command, "NAMES")) return SERVERMSG_NAMES;
	else if (!strcmp(Command, "WHO")) return SERVERMSG_WHO;
	else return SERVERMSG_UNKNOWN;
}


void Server_Loop(void)
{ //main loop for the NEXUS server.
	struct ClientList *Worker = NULL;
	struct NetReadReturn NRR;
	char MessageBuf[2048];
	
	Server_AcceptLoop();

LoopStart:
	Worker = ClientListCore; //We set it here and not at the initializer for a reason. Use your brain.

	for (; Worker; Worker = Worker->Next)
	{
		NRR = Net_Read(Worker->Descriptor, MessageBuf, sizeof MessageBuf, true);

#ifdef WIN
		if (NRR.Errno != 0 && NRR.Errno != WSAEWOULDBLOCK)
#else
		if (NRR.Status == 0 || (NRR.Status == -1 && NRR.Errno != EWOULDBLOCK))
#endif
		{ //Error. DELETE them.
			close(Worker->Descriptor); //Close the socket.
			Server_ClientList_Del(Worker->Descriptor);
			goto LoopStart;
		}
#ifdef WIN
		else if (NRR.Errno == WSAEWOULDBLOCK)
#else
		else if (NRR.Status == -1 && NRR.Errno == EWOULDBLOCK)
#endif
		{ //No data.
#ifdef WIN
			Sleep(1);
#else
			usleep(1000);
#endif
			continue;
		}
		
		//Get rid of expired scrollback.
		Scrollback_Reap();
		
		//We got data.
		NEXUS_NEXUS2IRC(MessageBuf, Worker);
		goto LoopStart; //We might have had our client deleted, so go to beginning.
	}
}
