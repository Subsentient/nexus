/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**This file controls ignored users and whatnot.**/

#include <stdio.h>
#include <stdlib.h>

#include "substrings/substrings.h"
#include "irc.h"
#include "ignore.h"
#include "nexus.h"
static struct IgnoreList *IgnoreCore;

static bool Ignore_Del(struct IgnoreList *Delete);

bool Ignore_Add(const char *Message, const unsigned WhatToBlock)
{ //Success if the broken down nick/ident/mask makes sense and can be added.

	struct IgnoreList *Worker = IgnoreCore;
	if (!IgnoreCore)
	{
		IgnoreCore = Worker = (struct IgnoreList*)CPSL_List_NewList(sizeof(struct IgnoreList));
	}
	else Worker = (struct IgnoreList*)CPSL_List_AddNode(&IgnoreCore->ListInfo);
	
	char Nick[sizeof ((struct IgnoreList*)0)->Nick];
	char Ident[sizeof ((struct IgnoreList*)0)->Ident];
	char Mask[sizeof ((struct IgnoreList*)0)->Mask];
	
	if (!IRC_BreakdownNick(Message, Nick, Ident, Mask))
	{ //Bad or corrupted input.
		return false;
	}
	
	
	SubStrings.Copy(Worker->Nick, Nick, sizeof Worker->Nick);
	SubStrings.Copy(Worker->Ident, Ident, sizeof Worker->Ident);
	SubStrings.Copy(Worker->Mask, Mask, sizeof Worker->Mask);
	
	Worker->WhatToBlock = WhatToBlock;
	
	return true;
}

bool Ignore_Check_Separate(const char *const Nick, const char *const Ident, const char *const Mask, const unsigned WhatToCheck)
{
	char TmpBuf[2048];
	
	snprintf(TmpBuf, sizeof TmpBuf, ":%s!%s@%s", Nick, Ident, Mask);
	
	return Ignore_Check(TmpBuf, WhatToCheck);
}

bool Ignore_Check(const char *Message, const unsigned WhatToCheck)
{ //See if a message originates/user is blocked.
	struct IgnoreList *Worker = IgnoreCore;
	
	if (!IgnoreCore) return false;
	
	char CheckNick[sizeof ((struct IgnoreList*)0)->Nick];
	char CheckIdent[sizeof ((struct IgnoreList*)0)->Ident];
	char CheckMask[sizeof ((struct IgnoreList*)0)->Mask];
	
	IRC_BreakdownNick(Message, CheckNick, CheckIdent, CheckMask);
	
	for (; Worker; Worker = CPSL_LNEXT(Worker))
	{
		if ((WhatToCheck & Worker->WhatToBlock) == WhatToCheck &&
			(*Worker->Nick == '*' || SubStrings.Compare(Worker->Nick, CheckNick)) &&
			(*Worker->Ident == '*' || SubStrings.Compare(Worker->Ident, CheckIdent)) &&
			(*Worker->Mask == '*' || SubStrings.Compare(Worker->Mask, CheckMask)))
		{
			return true;
		}
	}
	return false;
}

bool Ignore_Modify(const char *const VHost, const bool Adding, const unsigned WhatToChange)
{
	char Nick[sizeof ((struct IgnoreList*)0)->Nick];
	char Ident[sizeof ((struct IgnoreList*)0)->Ident];
	char Mask[sizeof ((struct IgnoreList*)0)->Mask];
	
	IRC_BreakdownNick(VHost, Nick, Ident, Mask);
	
	struct IgnoreList *Worker = IgnoreCore;
	
	
	for (; Worker; Worker = CPSL_LNEXT(Worker))
	{
		if (SubStrings.Compare(Nick, Worker->Nick) &&
			SubStrings.Compare(Ident, Worker->Ident) &&
			SubStrings.Compare(Mask, Worker->Mask))
		{
			//This is it.
			if (Adding)
			{
				if (Worker->WhatToBlock == (Worker->WhatToBlock | WhatToChange)) return false;
				
				Worker->WhatToBlock |= WhatToChange;
			}
			else
			{	
				Worker->WhatToBlock &= ~WhatToChange;
				
				if (!Worker->WhatToBlock)
				{
					Ignore_Del(Worker);
				}
			}
			return true;
		}
	}
	
	if (Adding)
	{ //Doesn't exist, create it.
		Ignore_Add(VHost, WhatToChange);
		return true;
	}
	
	return false;
}

void Ignore_Shutdown(void)
{
	if (!IgnoreCore) return;
	
	CPSL_List_DestroyList(&IgnoreCore->ListInfo);
	IgnoreCore = NULL;
}

static bool Ignore_Del(struct IgnoreList *Delete)
{
	if (!Delete) return false;
	
	if (!IgnoreCore) return false;	
	
	struct IgnoreList *NewHead = (struct IgnoreList*)CPSL_List_DeleteNode(&Delete->ListInfo);
	
	if (!NewHead) IgnoreCore = NULL;
	else IgnoreCore = NewHead;
	
	return true;
}

