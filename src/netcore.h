/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**Header file for network core.**/

#ifndef __NETCORE_HEADER__
#define __NETCORE_HEADER__

#include <stdbool.h>

//Structures

#define Net_Write(a, b, c) 		printf("Net_Write >> %s:%d\n", __FILE__, __LINE__), Net_Write_(a, b, c)

//Globals
extern int ServerDescriptor, IRCDescriptor;

//Function prototypes
bool Net_InitServer(unsigned short PortNum);
bool Net_Read(int Descriptor, void *OutStream_, unsigned MaxLength, bool IsText);
bool Net_Write_(int const ClientDescriptor, const void *InMsg, unsigned WriteSize);
void Net_ShutdownServer(void);
bool Net_Connect(const char *InHost, unsigned short PortNum, int *SocketDescriptor_);
bool Net_AcceptClient(int *const OutDescriptor, char *const OutIPAddr, unsigned IPAddrMaxLen);
bool Net_Close(const int Descriptor);

#endif //__NETCORE_HEADER__
