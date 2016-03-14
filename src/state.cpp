/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**Keeps track of channels and other state information.**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <list>

#include "substrings/substrings.h"
#include "state.h"
#include "nexus.h"

//Globals
std::map<std::string, ChannelList> ChannelListCore;

//Prototypes

//Functions
ChannelList *State::AddChannel(const char *const Channel)
{
	ChannelList NewChan(Channel);
	ChannelListCore[Channel] = NewChan;
	return &ChannelListCore[Channel];
}

class ChannelList *State::LookupChannel(const char *const ChannelName)
{
	std::map<std::string, class ChannelList>::iterator Iter = ChannelListCore.begin();
	
	for (; Iter != ChannelListCore.end(); ++Iter)
	{
		if (SubStrings.CaseCompare(Iter->first.c_str(), ChannelName))
		{ //As a C guy going to C++, some of this stuff like std::pair gives me a headache. Nonetheless, it's somewhat useful.
			return &Iter->second;
		}
	}
	
	return NULL;
}

bool State::DelChannel(const char *const Channel)
{
	return ChannelListCore.erase(Channel);
}


struct UserStruct *ChannelList::GetUser(const char *const Nick) const
{
	if (this->UserList.count(Nick) == 0) return NULL;
	
	return &const_cast<ChannelList*>(this)->UserList[Nick];
}

void ChannelList::WipeUsers(void)
{
	this->UserList.clear();
}

void State::ShutdownChannelList(void)
{
	ChannelListCore.clear();
}

unsigned char State::UserModes_Get_Symbol2Mode(const char Symbol)
{
	switch (Symbol)
	{
		case '!':
			return F_IRCMODE_IRCSTAFF;
		case '~':
			return F_IRCMODE_FOUNDER;
		case '&':
			return F_IRCMODE_PROTECTED;
		case '@':
			return F_IRCMODE_OP;
		case '%':
			return F_IRCMODE_HALFOP;
		case '+':
			return F_IRCMODE_VOICE;
		default:
			return 0;
	}
}

char State::UserModes_Get_Mode2Symbol(const unsigned char Modes)
{
	if (Modes & F_IRCMODE_IRCSTAFF) return '!';
	else if (Modes & F_IRCMODE_FOUNDER) return '~';
	else if (Modes & F_IRCMODE_PROTECTED) return '&';
	else if (Modes & F_IRCMODE_OP) return '@';
	else if (Modes & F_IRCMODE_HALFOP) return '%';
	else if (Modes & F_IRCMODE_VOICE) return '+';
	else return 0;
}

unsigned char State::UserModes_Get_Letter2Mode(char Letter)
{
	switch (Letter)
	{
		case 'y':
			return F_IRCMODE_IRCSTAFF;
		case 'q':
			return F_IRCMODE_FOUNDER;
		case 'a':
			return F_IRCMODE_PROTECTED;
		case 'o':
			return F_IRCMODE_OP;
		case 'h':
			return F_IRCMODE_HALFOP;
		case 'v':
			return F_IRCMODE_VOICE;
		default:
			return 0;
	}
}

bool ChannelList::AddUser(const char *const Nick, const unsigned char Modes)
{
	if (this->UserList.count(Nick) != 0) return false;
	
	struct UserStruct NewUser;
	NewUser.Nick = Nick;
	NewUser.Modes = Modes;
	
	this->UserList[Nick] = NewUser;
	
	return true;
}

bool ChannelList::RenameUser(const char *const OldNick, const char *const NewNick)
{
	struct UserStruct *User = this->GetUser(OldNick);
	
	if (!User) return false;
	
	const unsigned Modes = User->Modes;
	
	this->DelUser(OldNick);
	this->AddUser(NewNick, Modes);
	
	return true;
}

bool ChannelList::DelUser(const char *const Nick)
{
	if (this->UserList.count(Nick) == 0) return false;
	
	return this->UserList.erase(Nick);
}

void ChannelList::SetTopic(const char *InTopic)
{
	if (!InTopic)
	{
		this->Topic.clear();
		return;
	}
	
	this->Topic = InTopic;
}

void ChannelList::SetWhoSetTopic(const char *In)
{
	if (!In)
	{
		this->WhoSetTopic.clear();
		return;
	}
	
	this->WhoSetTopic = In;
}

void ChannelList::SetWhenSetTopic(const unsigned WhenSetTopic)
{
	this->WhenSetTopic = WhenSetTopic;
}
	



