/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#ifdef WIN
#include <winsock2.h>
#else
#include <fcntl.h>
#endif 

#include <list>
#include <string>

#include "server.h"
#include "netcore.h"
#include "config.h"
#include "nexus.h"
#include "state.h"
#include "scrollback.h"
#include "irc.h"

#include "substrings/substrings.h"

/**This file has our IRC pseudo-server that we run ourselves and its interaction with clients.**/

//Globals
std::list<struct ClientListStruct> ClientListCore;
struct ClientListStruct *CurrentClient, *PreviousClient;

//Prototypes
namespace Server
{
	static void SendChannelNamesList(const class ChannelList *const Channel, struct ClientListStruct *Client);
	static void SendChannelRejoin(const class ChannelList *const Channel, const int ClientDescriptor);
	static void SendIRCWelcome(const int ClientDescriptor);
}
//Functions

struct ClientListStruct *Server::ClientList::Lookup(const int Descriptor)
{
	std::list<struct ClientListStruct>::iterator Iter = ClientListCore.begin();

	for (; Iter != ClientListCore.end(); ++Iter)
	{
		if (Iter->Descriptor == Descriptor)
		{
			return &*Iter; //Not used to doing this kind of weird shit. That'd seem really stupid and redundant in C, where Iter would be a pointer.
		}
	}
	return NULL;
}

struct ClientListStruct *Server::ClientList::Add(const struct ClientListStruct *const InStruct)
{
	ClientListCore.push_front(*InStruct);
	return &*ClientListCore.begin();
}


void Server::ClientList::Shutdown(void)
{
	ClientListCore.clear();
	
	PreviousClient = NULL;
	CurrentClient = NULL;
}

bool Server::ClientList::Del(const int Descriptor)
{
	std::list<struct ClientListStruct>::iterator Iter = ClientListCore.begin();
	
	for (; Iter != ClientListCore.end(); ++Iter)
	{
		if (Iter->Descriptor == Descriptor)
		{
			if (&*Iter == CurrentClient) CurrentClient = reinterpret_cast<struct ClientListStruct*>(-1);
			if (&*Iter == PreviousClient) PreviousClient = reinterpret_cast<struct ClientListStruct*>(-1);
			
			ClientListCore.erase(Iter);
			return true;
		}
	}
	return false;
}

bool Server::ForwardToAll(const char *const InStream)
{ //This function sends the provided text stream to all clients. Very simple.
	std::list<struct ClientListStruct>::iterator Iter = ClientListCore.begin();
	
	for (; Iter != ClientListCore.end(); ++Iter)
	{
		Iter->SendLine(InStream);
	}
	return true;
}

bool Server::NukeClient(const int Descriptor)
{ //Close the descriptor, remove from select() tracking, and purge him from our minds.
	struct ClientListStruct *Client = Server::ClientList::Lookup(Descriptor);

	if (!Client) return false;
	
	Net::Close(Client->Descriptor);
	NEXUS::DescriptorSet_Del(Client->Descriptor);
	Server::ClientList::Del(Client->Descriptor);
	
	return true;

}

void Server::SendQuit(const int Descriptor, const char *const Reason)
{ //Tells all clients or just one client to quit
	std::list<struct ClientListStruct>::iterator Iter = ClientListCore.begin();
	char OutBuf[2048];
	
	for (; Iter != ClientListCore.end(); ++Iter)
	{
		//If not on "everyone" mode we check if the descriptor matches.
		if (Descriptor != -1 && Descriptor != Iter->Descriptor) continue; 
		
		if (Reason)
		{
			snprintf(OutBuf, sizeof OutBuf, ":%s!%s@%s QUIT :%s\r\n", IRCConfig.Nick, Iter->Ident, Iter->IP, Reason);
		}
		else
		{
			snprintf(OutBuf, sizeof OutBuf, ":%s!%s@%s QUIT :Disconnected from NEXUS.\r\n", IRCConfig.Nick, Iter->Ident, Iter->IP);
		}
		
		Iter->SendLine(OutBuf);
	}
}

static void Server::SendIRCWelcome(const int ClientDescriptor)
{
	char OutBuf[2048];
	struct ClientListStruct *Client = Server::ClientList::Lookup(ClientDescriptor);
	std::map<std::string, ChannelList>::iterator Iter = ChannelListCore.begin();
	
	const int ClientCount = ClientListCore.size();
	
	if (!Client) return;
	
	
	//First thing we send is our greeting, telling them they're connected OK.
	snprintf(OutBuf, sizeof OutBuf, ":" NEXUS_FAKEHOST " 001 %s :NEXUS is forwarding you to server %s:%hu\r\n",
			IRCConfig.Nick, IRCConfig.Server, IRCConfig.PortNum); //Putting IRCConfig.Nick here is the same as sending a NICK command.
			
	Client->SendLine(OutBuf);
	
	//Tell them to join all channels we are already in.
	for (; Iter != ChannelListCore.end(); ++Iter)
	{
		Server::SendChannelRejoin(&Iter->second, Client->Descriptor);
	}
	
	
	//Count clients for our next cool little trick.
	
	
	/**Send a MOTD.**/
	
	//Send the beginning.
	snprintf(OutBuf, sizeof OutBuf, ":" NEXUS_FAKEHOST " 375 %s :Welcome to NEXUS " NEXUS_VERSION
			". You are being forwarded to \"%s:%hu\".\r\n", IRCConfig.Nick, IRCConfig.Server, IRCConfig.PortNum);
	
	Client->SendLine(OutBuf);
	
	//For dumb bots that connect, make it abundantly clear what their nick should be.
	snprintf(OutBuf, sizeof OutBuf, ":%s!%s@%s NICK :%s\r\n", Client->OriginalNick, Client->Ident, Client->IP, IRCConfig.Nick);
	Client->SendLine(OutBuf);
	
	//Send the middle.
	snprintf(OutBuf, sizeof OutBuf,
			":" NEXUS_FAKEHOST " 372 %s :There are currently %d other instances connected to this NEXUS server.\r\n",
			IRCConfig.Nick, ClientCount - 1);
	Client->SendLine(OutBuf);
	
	//Send the end.
	snprintf(OutBuf, sizeof OutBuf, ":" NEXUS_FAKEHOST " 376 %s :End of MOTD.\r\n", IRCConfig.Nick);
	Client->SendLine(OutBuf);
	
	//Send all scrollback.
	struct ScrollbackList *SWorker = ScrollbackCore;
	char ScrollBuf[2048], SelfOrigin[256];
	
	snprintf(SelfOrigin, sizeof SelfOrigin, "%s!%s@%s", IRCConfig.Nick, Client->Ident, Client->IP);
	
	for (; SWorker; SWorker = SWorker->Next)
	{ //Loop through sending the scrollback.
		struct tm *TimeStruct = localtime(&SWorker->Time);
		char TimeBuf[128];
		const char *Origin = SWorker->Origin;
		
		strftime(TimeBuf, sizeof TimeBuf, "%I:%M:%S %p %Y-%m-%d", TimeStruct);

		if (!Origin)
		{ //We want to use ourselves as the origin if none specified.
			Origin = SelfOrigin;
		}
		
		if (SubStrings.StartsWith("\1ACTION ", SWorker->Msg))
		{
			char *NewMsg = (char*)calloc(strlen(SWorker->Msg) + 1, 1);
			
			//Don't bother trying to understand the arithmetic here.
			SubStrings.Copy(NewMsg, SWorker->Msg + (sizeof "\1ACTION " - 1), (strlen(SWorker->Msg) + 1) - (sizeof "\1ACTION " - 1));
			
			//Chop off the \x01 at the end.
			SubStrings.StripTrailingChars(NewMsg, "\001");
			
			//Build the message.
			char Nick[64], Ident[256], Mask[512];
			IRC::BreakdownNick(Origin, Nick, Ident, Mask);
			
			snprintf(ScrollBuf, sizeof ScrollBuf, ":%s PRIVMSG %s :\0034[%s]\3 ** %s %s **\r\n",
					Origin, (SWorker->Target ? SWorker->Target : IRCConfig.Nick), TimeBuf, Nick, NewMsg);
			free(NewMsg);
		}
		else
		{
			snprintf(ScrollBuf, sizeof ScrollBuf, ":%s PRIVMSG %s :\0034[%s]\3 %s\r\n",
				Origin, (SWorker->Target ? SWorker->Target : IRCConfig.Nick), TimeBuf, SWorker->Msg);
		}
		
		//Now send it to the client.
		Client->SendLine(ScrollBuf);
	}
	
}

static void Server::SendChannelNamesList(const class ChannelList *const Channel, struct ClientListStruct *Client)
{
	char OutBuf[2048];
	unsigned Ticker = 1;

	std::map<std::string, struct UserStruct> &UserListRef = Channel->GetUserList();
	std::map<std::string, struct UserStruct>::iterator Iter = UserListRef.begin();
	
SendBegin:
	
	std::string OutString = std::string(":" NEXUS_FAKEHOST " 353 ") + IRCConfig.Nick + " = " + Channel->GetChannelName() + " :";
	
	for (Ticker = 1; Iter != UserListRef.end(); ++Iter, ++Ticker)
	{
		const struct UserStruct *Worker = &Iter->second;
		
		//Reconstitute the mode flag for this user.
		const char Sym = State::UserModes_Get_Mode2Symbol(Worker->Modes);
		
		if (Sym)
		{
			OutString += Sym;
		}
		
		OutString += Worker->Nick;

		std::map<std::string, struct UserStruct>::iterator TempIter = Iter;
		++TempIter;
		
		if (Ticker == 20 || TempIter == UserListRef.end())
		{
			OutString += "\r\n";
			Client->SendLine(OutString.c_str());
			
			if (TempIter != UserListRef.end())
			{
				goto SendBegin;
			}
		}
		else
		{
			OutString += " ";
		}
	}
	
	snprintf(OutBuf, sizeof OutBuf, ":" NEXUS_FAKEHOST " 366 %s %s :End of /NAMES list.\r\n", IRCConfig.Nick, Channel->GetChannelName());
	Client->SendLine(OutBuf);
	
}

static void Server::SendChannelRejoin(const class ChannelList *const Channel, const int ClientDescriptor)
{ //Sends the list of channels we are in to the client specified.
	char OutBuf[2048];
	struct ClientListStruct *Client = Server::ClientList::Lookup(ClientDescriptor);
	
	if (!Client) return;
	
	//Send the join command.
	snprintf(OutBuf, sizeof OutBuf, ":%s!%s@%s JOIN %s\r\n", IRCConfig.Nick, Client->Ident, Client->IP, Channel->GetChannelName());
	Client->SendLine(OutBuf);
	
	//Send the topic and the setter of the topic.
	if (*Channel->GetTopic() && *Channel->GetWhoSetTopic() && Channel->GetWhenSetTopic() != 0)
	{
		snprintf(OutBuf, sizeof OutBuf, ":" NEXUS_FAKEHOST " 332 %s %s :%s\r\n", IRCConfig.Nick, Channel->GetChannelName(), Channel->GetTopic());
		Client->SendLine(OutBuf);
		
		snprintf(OutBuf, sizeof OutBuf, ":" NEXUS_FAKEHOST " 333 %s %s %s %u\r\n", IRCConfig.Nick,
				Channel->GetChannelName(), Channel->GetWhoSetTopic(), Channel->GetWhenSetTopic());
		Client->SendLine(OutBuf);
	}
	
	//Send list of users.
	Server::SendChannelNamesList(Channel, Client);
}

struct ClientListStruct *Server::AcceptLoop(void)
{
	struct ClientListStruct TempClient;
	char InBuf[2048];
	struct ClientListStruct *Client = NULL;
	bool UserProvided = false, NickProvided = false;
	bool PasswordProvided = false;
	
	if (!Net::AcceptClient(&TempClient.Descriptor, TempClient.IP, sizeof TempClient.IP))
	{ //No client.
		return NULL;
	}


	///Apparently there is a client.
	Client = Server::ClientList::Add(&TempClient); //Store their information.

	/**Continuously try to read their replies until we get them.**/
	while (!UserProvided || !NickProvided)
	{ //Wait for their greeting.
		const bool NetReadReturn = Net::Read(Client->Descriptor, InBuf, sizeof InBuf, true);
		
		if (!NetReadReturn)
		{
			Server::NukeClient(Client->Descriptor);
			return NULL;
		}
		
		//Does it start with USER?
		if (!strncmp(InBuf, "USER", sizeof "USER" - 1) || !strncmp(InBuf, "user", sizeof "user" - 1))
		{ //This information is needed to fool the IRC clients.
			const char *TWorker = InBuf + sizeof "USER";
			unsigned Inc = 0;
			
			//If we want a password, WE WANT A PASSWORD. You're supposed to send PASS first, dunce!
			if (*NEXUSConfig.ServerPassword && !PasswordProvided)
			{
				Server::NukeClient(Client->Descriptor);
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
				Net::Close(Client->Descriptor);
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
				Net::Close(Client->Descriptor);
				return NULL;
			}
		}
		continue;
	}
	
	//Get rid of expired scrollback before we send it to someone.
	Scrollback::Reap();
	
	//Time to welcome them.
	Server::SendIRCWelcome(Client->Descriptor);
	
	//Return the client we found.
	return Client;
}


enum ServerMessageType Server::GetMessageType(const char *InStream_)
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

bool ClientListStruct::Ping()
{
	std::string Out = "PING :" NEXUS_FAKEHOST "\r\n";
	bool RetVal = false;
#ifdef WIN
	u_long Value = 1;
	ioctlsocket(this->Descriptor, FIONBIO, &Value);
#else
	fcntl(this->Descriptor, F_SETFL, O_NONBLOCK); //Set nonblocking. Necessary for our single-threaded model.
#endif //WIN
	
	try
	{
		Net::Write(this->Descriptor, Out.c_str(), Out.length());
	}
	catch (Net::Errors::Any)
	{
		goto End;
	}
	
	RetVal = true;
	
	this->PingSentTime = time(NULL);
	this->WaitingForPing = true;
End:
#ifdef WIN
	Value = 0;
	ioctlsocket(this->Descriptor, FIONBIO, &Value);
#else
	fcntl(this->Descriptor, F_SETFL, 0);
#endif //WIN
	return RetVal;
}

bool ClientListStruct::CompletePing(void)
{
	if (!this->WaitingForPing) return false;
	
	this->WaitingForPing = false;
	this->PingRecvTime = time(NULL);
	
	return true;
}

ClientListStruct::ClientListStruct(const int InDescriptor, const char *IP, const char *OriginalNick, const char *Ident)
	: WaitingForPing(false), PingSentTime(0), PingRecvTime(time(NULL)), Descriptor(InDescriptor)
{ //PingRecvTime is initialized with a real time so that the server doesn't send pings instantly.
	SubStrings.Copy(this->IP, IP, sizeof this->IP);
	SubStrings.Copy(this->OriginalNick, OriginalNick, sizeof this->OriginalNick);
	SubStrings.Copy(this->Ident, Ident, sizeof this->Ident);
}
	
ClientListStruct::ClientListStruct(void)
	: WaitingForPing(false), PingSentTime(0), PingRecvTime(time(NULL)), Descriptor(0)
{
}


void ClientListStruct::SendLine(const char *const String)
{
	std::string Out = String;
	if (!SubStrings.EndsWith("\r\n", Out.c_str()))
	{
		Out += "\r\n";
	}
	
	this->WriteQueue.push(Out);
}

bool ClientListStruct::FlushSendBuffer(void)
{
	if (this->WriteQueue.empty()) return false;
	
	while (!this->WriteQueue.empty())
	{
		if (!this->WriteQueue_Pop()) return false;
	}
	
	return true;
}

bool ClientListStruct::WriteQueue_Pop(void)
{
	if (this->WriteQueue.empty()) return false;
	
	bool RetVal = false;
	std::string Out = this->WriteQueue.front();
	
#ifdef WIN
	u_long Value = 1;
	ioctlsocket(this->Descriptor, FIONBIO, &Value);
#else
	fcntl(this->Descriptor, F_SETFL, O_NONBLOCK); //Set nonblocking. Necessary for our single-threaded model.
#endif //WIN

	try
	{
		Net::Write(this->Descriptor, Out.c_str(), Out.length());
	}
	catch (Net::Errors::BlockingError Err)
	{
		if (Err.BytesSentBeforeBlocking > 0)
		{
			std::string &Str = this->WriteQueue.front();
			std::string New = Str;
			
			Str = New.c_str() + Err.BytesSentBeforeBlocking;
		}
		goto End;
	}
	catch (Net::Errors::Any)
	{ //We will probably get Net::Errors::BlockingError.
		goto End;
	}
	
	RetVal = true;
	this->WriteQueue.pop();
		
End:
#ifdef WIN
	Value = 0;
	ioctlsocket(this->Descriptor, FIONBIO, &Value);
#else
	fcntl(this->Descriptor, F_SETFL, 0);
#endif //WIN
	return RetVal;
}
