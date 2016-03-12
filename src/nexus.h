/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**Main header file for NEXUS.**/

#ifndef __NEXUS_HEADER__
#define __NEXUS_HEADER__

#ifdef DEBUG
#define Exit(Code) printf("Exit code %s at %s:%d\n", #Code, __FILE__, __LINE__); exit(Code)
#else
#define Exit exit
#endif //DEBUG

#include <stddef.h>
#include "server.h"

#define NEXUS_VERSION "0.1"
#define CONTROL_NICKNAME "NEXUS_CONTROL"

void NEXUS_Loop(void);
void NEXUS_IRC2NEXUS(const char *Message);
void NEXUS_NEXUS2IRC(const char *Message, struct ClientList *const Client);
void NEXUS_DescriptorSet_Add(const int Descriptor);
bool NEXUS_DescriptorSet_Del(const int Descriptor);
#endif //__NEXUS_HEADER__
