/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**Header file for network core.**/

#ifndef __NETCORE_HEADER__
#define __NETCORE_HEADER__

#include <stdbool.h>

//Structures
struct ClientTree
{ //Contains information about what clients are who.
	int Descriptor; //Network descriptor for this guy.
	char IP[128]; //His IP address.
	
	struct ClientTree *Next, *Prev;
};

struct NetReadReturn
{
	int Status;
	int Errno;
};

//Globals
extern int ServerDescriptor, IRCDescriptor;

//Function prototypes
bool Net_InitServer(unsigned short PortNum);
struct NetReadReturn Net_Read(int ClientDescriptor, void *OutStream_, unsigned MaxLength, bool IsText);
bool Net_Write(int const ClientDescriptor, void *InMsg, unsigned WriteSize);
void Net_ShutdownServer(void);
bool Net_Connect(const char *InHost, unsigned short PortNum, int *SocketDescriptor_);

#endif //__NETCORE_HEADER__
