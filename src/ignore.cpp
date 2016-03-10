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
	
	char Nick[sizeof ((struct IgnoreList*)0)->Nick];
	char Ident[sizeof ((struct IgnoreList*)0)->Ident];
	char Mask[sizeof ((struct IgnoreList*)0)->Mask];
	
	if (!IRC_BreakdownNick(Message, Nick, Ident, Mask))
	{ //Bad or corrupted input.
		return false;
	}
	
	if (!Worker)
	{
		IgnoreCore = Worker = (struct IgnoreList*)calloc(1, sizeof(struct IgnoreList));
	}
	else
	{
		while (Worker->Next) Worker = Worker->Next;
		
		Worker->Next = (struct IgnoreList*)calloc(1, sizeof(struct IgnoreList));
		Worker->Next->Prev = Worker;
		Worker = Worker->Next;
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
	
	char CheckNick[sizeof ((struct IgnoreList*)0)->Nick];
	char CheckIdent[sizeof ((struct IgnoreList*)0)->Ident];
	char CheckMask[sizeof ((struct IgnoreList*)0)->Mask];
	
	IRC_BreakdownNick(Message, CheckNick, CheckIdent, CheckMask);
	
	for (; Worker; Worker = Worker->Next)
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
	
	
	for (; Worker; Worker = Worker->Next)
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
	struct IgnoreList *Worker = IgnoreCore, *Next = NULL;
	
	for (; Worker; Worker = Next)
	{
		Next = Worker->Next;
		free(Worker);
	}
	IgnoreCore = NULL;
}

static bool Ignore_Del(struct IgnoreList *Delete)
{
	
	if (Delete == IgnoreCore)
	{
		if (IgnoreCore->Next)
		{
			IgnoreCore = IgnoreCore->Next;
			IgnoreCore->Prev = NULL;
			free(Delete);
		}
		else
		{
			IgnoreCore = NULL;
			free(Delete);
		}
	}
	else
	{
		Delete->Prev->Next = Delete->Next;
		if (Delete->Next) Delete->Next->Prev = Delete->Prev;
		free(Delete);
	}
	
	return true;
}

