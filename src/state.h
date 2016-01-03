/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**header file for state.c**/

#ifndef __STATE_HEADER__
#define __STATE_HEADER__

#include <stdbool.h>

//Flags.
#define F_IRCMODE_VOICE 0x1
#define F_IRCMODE_HALFOP 0x2
#define F_IRCMODE_OP 0x4
#define F_IRCMODE_PROTECTED 0x8
#define F_IRCMODE_FOUNDER 0x10
#define F_IRCMODE_IRCSTAFF 0x20

//Structures

struct _UserList
{ 
	char Nick[64]; //The nick.
	unsigned char Modes; //Modes on the user, such as +o or +v.
	struct _UserList *Next, *Prev;
};

struct ChannelList
{ //Holds channels and recursively, a list of users for each channel.
	char Channel[256]; //Name of the channel.
	char Topic[1024]; //Current topic of the channel.
	char WhoSetTopic[256]; //Who set it. In the form of nick!ident@mask usually.
	unsigned WhenSetTopic; //When the topic was set, in UNIX time.
	struct _UserList *UserList; //List of users and whatnot.

	
	struct ChannelList *Next, *Prev;
};


//Functions
bool State_DelChannel(const char *const Channel);
struct ChannelList *State_AddChannel(const char *const Channel);
void State_ShutdownChannelList(void);
bool State_DelUserFromChannel(const char *Nick, struct ChannelList *Channel);
struct _UserList *State_AddUserToChannel(const char *Nick, const unsigned char Modes, struct ChannelList *Channel);
struct ChannelList *State_LookupChannel(const char *const ChannelName);
struct _UserList *State_GetUserInChannel(const char *Nick, struct ChannelList *Channel);
char State_UserModes_Get_Mode2Symbol(const unsigned char Modes);
unsigned char State_UserModes_Get_Symbol2Mode(const char Symbol);
unsigned char State_UserModes_Get_Letter2Mode(char Letter);

//Globals
extern struct ChannelList *ChannelListCore;

#endif //__STATE_HEADER__
