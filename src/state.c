/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**Keeps track of channels, scrollback, and other state information.**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "state.h"

//Globals
struct ChannelList *ChannelListCore;

//Prototypes
static void State_DelAllChannelUsers(struct ChannelList *Channel);

//Functions
struct ChannelList *State_AddChannel(const char *const Channel)
{
	struct ChannelList *Worker = ChannelListCore;
	
	if (!ChannelListCore)
	{ //We're the first.
		Worker = ChannelListCore = calloc(1, sizeof(struct ChannelList));
	}
	else
	{ //Not the first.
		
		for (; Worker; Worker = Worker->Next)
		{ //Check for duplicates.
			if (!strcmp(Worker->Channel, Channel))
			{
				return Worker;
			}
		}
		
		//Skip to a free Next.
		for (Worker = ChannelListCore; Worker->Next; Worker = Worker->Next);
		
		Worker->Next = calloc(1, sizeof(struct ChannelList));
		Worker->Next->Prev = Worker;
		Worker = Worker->Next;
	}
	
	//Copy in that channel name.
	strncpy(Worker->Channel, Channel, sizeof Worker->Channel - 1);
	Worker->Channel[sizeof Worker->Channel - 1] = '\0';
	
	return Worker;
}

bool State_DelChannel(const char *const Channel)
{
	struct ChannelList *Worker = ChannelListCore;
	
	for (; Worker; Worker = Worker->Next)
	{
		if (!strcmp(Channel, Worker->Channel))
		{
			//Delete all users first.
			State_DelAllChannelUsers(Worker);
			
			//Then delete the channel.
			if (Worker == ChannelListCore)
			{
				if (Worker->Next)
				{
					ChannelListCore = Worker->Next;
					free(Worker);
				}
				else
				{
					ChannelListCore = NULL;
					free(Worker);
				}
			}
			else
			{
				Worker->Prev->Next = Worker->Next;
				if (Worker->Next) Worker->Next->Prev = Worker->Prev;
				free(Worker);
			}
			return true;
		}
	}
	return false;
}

void State_ShutdownChannelList(void)
{
	struct ChannelList *Worker = ChannelListCore, *Next;
	
	for (; Worker; Worker = Next)
	{
		Next = Worker->Next;
		free(Worker);
	}
}

struct UserList *State_AddUserToChannel(const char *Nick, const char Symbol, struct ChannelList *Channel)
{
	struct UserList *Worker = Channel->UserList;
	
	if (!Worker)
	{
		Worker = Channel->UserList = calloc(1, sizeof(struct UserList));
	}
	else
	{
		for (; Worker; Worker = Worker->Next)
		{ //Check for dupes.
			if (!strcmp(Worker->Nick, Nick))
			{
				Worker->Symbol = Symbol; //Update the symbol, might as well.
				return Worker;
			}
		}
		
		//Skip to a NULL Next.
		for (Worker = Channel->UserList; Worker->Next; Worker = Worker->Next);
		
		Worker->Next = calloc(1, sizeof(struct UserList));
		Worker->Next->Prev = Worker;
		Worker = Worker->Next;
	}
	
	//Copy in the nickname.
	strncpy(Worker->Nick, Nick, sizeof Worker->Nick - 1);
	Worker->Nick[sizeof Worker->Nick - 1] = '\0';
	
	//Then set the user's symbol.
	Worker->Symbol = Symbol;
	
	return Worker;
}

struct ChannelList *State_LookupChannel(const char *const ChannelName)
{
	struct ChannelList *Worker = ChannelListCore;
	
	for (; Worker; Worker = Worker->Next)
	{
		if (!strcmp(ChannelName, Worker->Channel))
		{
			return Worker;
		}
	}
	
	return NULL;
}

struct UserList *State_GetUserInChannel(const char *Nick, struct ChannelList *Channel)
{ //Looks up the user structure for the specified nick in the specified channel.
	struct UserList *Worker = Channel->UserList;
	
	for (; Worker; Worker = Worker->Next)
	{
		if (!strcmp(Nick, Worker->Nick))
		{
			return Worker;
		}
	}
	
	return NULL;
}


bool State_DelUserFromChannel(const char *Nick, struct ChannelList *Channel)
{
	struct UserList *Worker = Channel->UserList;
	
	for (; Worker; Worker = Worker->Next)
	{
		if (!strcmp(Nick, Worker->Nick))
		{
			if (Worker == Channel->UserList)
			{
				if (Worker->Next)
				{
					Channel->UserList = Worker->Next;
					free(Worker);
				}
				else
				{
					Channel->UserList = NULL;
					free(Worker);
				}
			}
			else
			{
				Worker->Prev->Next = Worker->Next;
				if (Worker->Next) Worker->Next->Prev = Worker->Prev;
				free(Worker);
			}
			
			return true;
		}
	}
	return false;
}

static void State_DelAllChannelUsers(struct ChannelList *Channel)
{
	struct UserList *Worker = Channel->UserList, *Next;
	
	for (; Worker; Worker = Next)
	{
		Next = Worker->Next;
		free(Worker);
	}
}

