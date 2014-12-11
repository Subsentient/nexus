/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

///Header for server.c

#ifndef __SERVER_HEADER__
#define __SERVER_HEADER__

#include <stdbool.h>
//Structures
struct ClientTree
{ //Contains information about what clients are who.
	int Descriptor; //Network descriptor for this guy.
	char IP[128]; //His IP address.
	char OriginalNick[64]; //The nick they had before we told them to change it.
	char Ident[64]; //They get to keep their ident and we will call them by it.
	
	struct ClientTree *Next, *Prev;
};

//Prototypes.
struct ClientTree *Server_ClientTree_Lookup(const int Descriptor);
struct ClientTree *Server_ClientTree_Add(const struct ClientTree *const InStruct);
bool Server_ClientTree_Del(const int Descriptor);
void Server_ClientTree_Shutdown(void);
bool Server_ForwardToAll(const char *const InStream);
void Server_Loop(void);

//Globals
extern struct ClientTree *ClientTreeCore;

#endif //__SERVER_HEADER__
