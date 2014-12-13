/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**Main header file for NEXUS.**/

#ifndef __NEXUS_HEADER__
#define __NEXUS_HEADER__

#include <stdbool.h>
#include "server.h"

void NEXUS_Loop(void);
void NEXUS_IRC2NEXUS(const char *Message);
void NEXUS_NEXUS2IRC(const char *Message, struct ClientList *const Client);

#endif //__NEXUS_HEADER__
