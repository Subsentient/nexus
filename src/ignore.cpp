/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**This file controls ignored users and whatnot.**/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "substrings/substrings.h"
#include "irc.h"
#include "ignore.h"
#include "nexus.h"

#define IGNOREDB_FILE "ignore.db"

std::string IgnoreDBFile = IGNOREDB_FILE;

struct IgnoreList
{
	char Nick[64], Ident[64], Mask[64];
	unsigned WhatToBlock;
};

static std::list<struct IgnoreList> IgnoreCore;

namespace Ignore
{
	static bool Add(const char *Message, const unsigned WhatToBlock);
}


void Ignore::SendIgnoreList(struct ClientListStruct *Client)
{
	std::list<struct IgnoreList>::iterator Iter = IgnoreCore.begin();
	
	if (IgnoreCore.empty())
	{
		Client->SendNxCtlPrivmsg("No ignored vhosts found.");
		return;
	}
	
	for (; Iter != IgnoreCore.end(); ++Iter)
	{
		std::string Out = std::string(Iter->Nick) + "!" + Iter->Ident + "@" + Iter->Mask + ": ";
		
		if ((Iter->WhatToBlock & NEXUS_IGNORE_ALL) == NEXUS_IGNORE_ALL)
		{
			Out += "ALL";
			goto Jumpy;
		}
	
		if ((Iter->WhatToBlock & NEXUS_IGNORE_PRIVMSG) == NEXUS_IGNORE_PRIVMSG)
		{
			Out += "PRIVMSG,";
		}
		if ((Iter->WhatToBlock & NEXUS_IGNORE_CHANMSG) == NEXUS_IGNORE_CHANMSG)
		{
			Out += "CHANMSG,";
		}
		if ((Iter->WhatToBlock & NEXUS_IGNORE_NOTICE) == NEXUS_IGNORE_NOTICE)
		{
			Out += "NOTICE,";
		}
		if ((Iter->WhatToBlock & NEXUS_IGNORE_VISIBLE) == NEXUS_IGNORE_VISIBLE)
		{
			Out += "VISIBLE,";
		}
		
		if (Out[Out.length() - 1] == ',') Out[Out.length() - 1] = '\0';
	Jumpy:
		Client->SendNxCtlPrivmsg(Out.c_str());
	}
}
static bool Ignore::Add(const char *Message, const unsigned WhatToBlock)
{ //Success if the broken down nick/ident/mask makes sense and can be added.
	
	char Nick[64];
	char Ident[64];
	char Mask[128];
	
	if (!IRC::BreakdownNick(Message, Nick, Ident, Mask))
	{ //Bad or corrupted input.
		return false;
	}
	
	struct IgnoreList Temp;
	Temp.WhatToBlock = WhatToBlock;
	SubStrings.Copy(Temp.Nick, Nick, sizeof Temp.Nick);
	SubStrings.Copy(Temp.Ident, Ident, sizeof Temp.Ident);
	SubStrings.Copy(Temp.Mask, Mask, sizeof Temp.Mask);
		
	IgnoreCore.push_back(Temp);
	return true;
}

bool Ignore::Check_Separate(const char *const Nick, const char *const Ident, const char *const Mask, const unsigned WhatToCheck)
{
	char TmpBuf[2048];
	
	snprintf(TmpBuf, sizeof TmpBuf, ":%s!%s@%s", Nick, Ident, Mask);
	
	return Ignore::Check(TmpBuf, WhatToCheck);
}

bool Ignore::Check(const char *Message, const unsigned WhatToCheck)
{ //See if a message originates/user is blocked.
	std::list<struct IgnoreList>::iterator Iter = IgnoreCore.begin();
	
	char CheckNick[64];
	char CheckIdent[64];
	char CheckMask[128];
	
	IRC::BreakdownNick(Message, CheckNick, CheckIdent, CheckMask);
	
	for (; Iter != IgnoreCore.end(); ++Iter)
	{
		if ((WhatToCheck & Iter->WhatToBlock) == WhatToCheck &&
			(*Iter->Nick == '*' || SubStrings.Compare(Iter->Nick, CheckNick)) &&
			(*Iter->Ident == '*' || SubStrings.Compare(Iter->Ident, CheckIdent)) &&
			(*Iter->Mask == '*' || SubStrings.Compare(Iter->Mask, CheckMask)))
		{
			return true;
		}
	}
	return false;
}

bool Ignore::Modify(const char *const VHost, const bool Adding, const unsigned WhatToChange)
{
	char Nick[64];
	char Ident[64];
	char Mask[128];
	
	IRC::BreakdownNick(VHost, Nick, Ident, Mask);
	
	std::list<struct IgnoreList>::iterator Iter = IgnoreCore.begin();
	
	
	for (; Iter != IgnoreCore.end(); ++Iter)
	{
		if (SubStrings.Compare(Nick, Iter->Nick) &&
			SubStrings.Compare(Ident, Iter->Ident) &&
			SubStrings.Compare(Mask, Iter->Mask))
		{
			//This is it.
			if (Adding)
			{
				if (Iter->WhatToBlock == (Iter->WhatToBlock | WhatToChange)) return false;
				
				Iter->WhatToBlock |= WhatToChange;
			}
			else
			{	
				Iter->WhatToBlock &= ~WhatToChange;
				
				if (!Iter->WhatToBlock)
				{
					IgnoreCore.erase(Iter);
				}
			}
			return true;
		}
	}
	
	if (Adding)
	{ //Doesn't exist, create it.
		Ignore::Add(VHost, WhatToChange);
		return true;
	}
	
	return false;
}

bool Ignore::SaveDB(void)
{	
	if (IgnoreCore.empty())
	{ //No data, delete the file.
		return !remove(IgnoreDBFile.c_str());
	}
	
	FILE *Desc = fopen(IgnoreDBFile.c_str(), "wb");
	
	if (!Desc) return false;
	
	std::list<struct IgnoreList>::iterator Iter = IgnoreCore.begin();
	
	for (; Iter != IgnoreCore.end(); ++Iter)
	{
		fwrite(&*Iter, sizeof(struct IgnoreList), 1, Desc);
	}
	
	fclose(Desc);
	return true;
	
}

void Ignore::LoadDB(void)
{
	if (!IgnoreCore.empty()) Ignore::Shutdown(); //Wipe it first.
	
	struct stat FileStat;
	FILE *Desc = NULL;
	
	
	if (stat(IgnoreDBFile.c_str(), &FileStat) != 0 || !(Desc = fopen(IgnoreDBFile.c_str(), "rb")))
	{
		return;
	}
	
	if (FileStat.st_size == 0) //Empty.
	{
		fclose(Desc);
		return;
	}
	const unsigned NumElements = FileStat.st_size / sizeof(struct IgnoreList);	
	
	for (unsigned Inc = 0; Inc < NumElements; ++Inc)
	{
		struct IgnoreList Temp;
		fread(&Temp, sizeof(struct IgnoreList), 1, Desc);
#ifdef DEBUG
		printf("Ignore table specified and loaded for %s!%s@%s.\n", Temp.Nick, Temp.Ident, Temp.Mask);
#endif //DEBUG
		IgnoreCore.push_back(Temp);
	}
	
	fclose(Desc);
}

void Ignore::Shutdown(void)
{
	IgnoreCore.clear();
}

