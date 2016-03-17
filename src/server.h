/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

///Header for server.c

#ifndef __SERVER_HEADER__
#define __SERVER_HEADER__

#include <stdbool.h>

#include <list>
#include <string>
#include <queue>

#define NEXUS_FAKEHOST "NEXUSBNC_SERVER"

namespace NEXUS
{ //We need it for friend declarations in ClientListStruct
	void ProcessIdleActions(void);
	void MasterLoop(void);
}

//Structures
struct ClientListStruct
{ //Contains information about what clients are who.
private:
	std::queue<std::string> WriteQueue;
	bool WaitingForPing;
	time_t PingSentTime; //We use this for both checking if they pinged out and checking when we should ping them next.
	
	//Private member functions.
	bool WriteQueue_Pop(void);
	bool FlushSendBuffer(void);
public:
	int Descriptor; //Network descriptor for this guy.
	char IP[128]; //His IP address.
	char OriginalNick[64]; //The nick they had before we told them to change it.
	char Ident[64]; //They get to keep their ident and we will call them by it.
	
	ClientListStruct(void);	
	ClientListStruct(const int Descriptor, const char *IP = "", const char *OriginalNick = "", const char *Ident = "");
		
	///Safe function: Adds \r\n to end of the string before sending if we do not.
	void SendLine(const char *const String);
	bool Ping(void);
	bool CompletePing(void);
	
	friend void NEXUS::ProcessIdleActions(void);
	friend void NEXUS::MasterLoop(void);
};

enum ServerMessageType
{
	SERVERMSG_INVALID = -1,
	SERVERMSG_UNKNOWN,
	SERVERMSG_PRIVMSG,
	SERVERMSG_NOTICE,
	SERVERMSG_MODE,
	SERVERMSG_JOIN,
	SERVERMSG_PART,
	SERVERMSG_PING,
	SERVERMSG_PONG,
	SERVERMSG_QUIT,
	SERVERMSG_KICK,
	SERVERMSG_KILL,
	SERVERMSG_NICK,
	SERVERMSG_INVITE,
	SERVERMSG_TOPIC,
	SERVERMSG_NAMES,
	SERVERMSG_WHO
};

//Prototypes.
namespace Server
{
	namespace ClientList
	{
		struct ClientListStruct *Lookup(const int Descriptor);
		struct ClientListStruct *Add(const struct ClientListStruct *const InStruct);
		bool Del(const int Descriptor);
		void Shutdown(void);
	}
	bool ForwardToAll(const char *const InStream);
	struct ClientListStruct *AcceptLoop(void);
	void SendQuit(const int Descriptor, const char *const Reason);
	enum ServerMessageType GetMessageType(const char *InStream_);
	bool NukeClient(const int Descriptor);
}

//Globals
extern std::list<struct ClientListStruct> ClientListCore;
extern struct ClientListStruct *CurrentClient, *PreviousClient;

#endif //__SERVER_HEADER__
