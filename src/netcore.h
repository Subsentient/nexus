/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**Header file for network core.**/

#ifndef __NETCORE_HEADER__
#define __NETCORE_HEADER__

#include <stdbool.h>

//Structures
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
bool Net_AcceptClient(int *const OutDescriptor, char *const OutIPAddr, unsigned IPAddrMaxLen);

#endif //__NETCORE_HEADER__
