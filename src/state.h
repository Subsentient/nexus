/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**header file for state.c**/

#ifndef __STATE_HEADER__
#define __STATE_HEADER__

#include <stdbool.h>
//Structures

struct ChannelList
{ //Holds channels and recursively, a list of users for each channel.
	char Channel[256]; //Name of the channel.
	
	struct UserList
	{ //List of users and whatnot.
		char Nick[64]; //The nick.
		char Symbol; //Like if they're an operator we put the @ sign in front. Generally just gonna be \0.
		struct UserList *Next, *Prev;
	} *UserList;
	
	struct ChannelList *Next, *Prev;
};


//Functions
bool State_DelChannel(const char *const Channel);
struct ChannelList *State_AddChannel(const char *const Channel);
void State_ShutdownChannelList(void);
bool State_DelUserFromChannel(const char *Nick, struct ChannelList *Channel);
struct UserList *State_AddUserToChannel(const char *Nick, const char Symbol, struct ChannelList *Channel);
void State_DelAllChannelUsers(struct ChannelList *Channel);
struct ChannelList *State_LookupChannel(const char *const ChannelName);
struct UserList *State_GetUserInChannel(const char *Nick, struct ChannelList *Channel);

//Globals
extern struct ChannelList *ChannelListCore;

#endif //__STATE_HEADER__
