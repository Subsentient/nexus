/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

///Header for irc.c.

#ifndef __IRC_HEADER__
#define __IRC_HEADER__

#include <stdbool.h>

#define IRC_CODE_OK 1
#define IRC_CODE_NICKTAKEN 433

//Functions
bool IRC_GetStatusCode(const char *Message, int *OutNumber);
bool IRC_Connect(void);
void IRC_NickChange(const char *Nick);

#endif //__IRC_HEADER__
