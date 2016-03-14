/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

///Header for server.c

#ifndef __SERVER_HEADER__
#define __SERVER_HEADER__

#include <stdbool.h>

#include <list>

#define NEXUS_FAKEHOST "NEXUS"

//Structures
struct ClientListStruct
{ //Contains information about what clients are who.
	int Descriptor; //Network descriptor for this guy.
	char IP[128]; //His IP address.
	char OriginalNick[64]; //The nick they had before we told them to change it.
	char Ident[64]; //They get to keep their ident and we will call them by it.
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
struct ClientListStruct *Server_ClientList_Lookup(const int Descriptor);
struct ClientList *Server_ClientList_Add(const struct ClientList *const InStruct);
bool Server_ClientList_Del(const int Descriptor);
void Server_ClientList_Shutdown(void);
bool Server_ForwardToAll(const char *const InStream);
struct ClientListStruct *Server_AcceptLoop(void);
void Server_SendQuit(const int Descriptor, const char *const Reason);
enum ServerMessageType Server_GetMessageType(const char *InStream_);
bool Server_NukeClient(const int Descriptor);

//Globals
extern std::list<struct ClientListStruct> ClientListCore;
extern struct ClientListStruct *CurrentClient, *PreviousClient;

#endif //__SERVER_HEADER__
