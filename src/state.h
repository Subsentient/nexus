/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**header file for state.c**/

#ifndef __STATE_HEADER__
#define __STATE_HEADER__

#include <stdbool.h>
#include <list>
#include <string>
#include <map>
//Flags.
#define F_IRCMODE_VOICE 0x1
#define F_IRCMODE_HALFOP 0x2
#define F_IRCMODE_OP 0x4
#define F_IRCMODE_PROTECTED 0x8
#define F_IRCMODE_FOUNDER 0x10
#define F_IRCMODE_IRCSTAFF 0x20

//Structures

struct UserStruct
{ 
	std::string Nick; //The nick.
	unsigned char Modes; //Modes on the user, such as +o or +v.
};

class ChannelList
{ //Holds channels and recursively, a list of users for each channel.
private:
	std::string Channel; //Name of the channel.
	std::string Topic; //Current topic of the channel.
	std::string WhoSetTopic; //Who set it. In the form of nick!ident@mask usually.
	unsigned WhenSetTopic; //When the topic was set, in UNIX time.
	std::map<std::string, struct UserStruct> UserList; //List of users and whatnot.
public:
	inline ChannelList(std::string InChannel = "", std::string InTopic = "", std::string InWhoSetTopic = "", const unsigned WhenSetTopic = 0)
		: Channel(InChannel), Topic(InTopic), WhoSetTopic(InWhoSetTopic), WhenSetTopic(0) {}
	inline bool operator==(const ChannelList &In) const
	{
		return In.Channel == this->Channel;
	}
	bool AddUser(const char *Nick, const unsigned char Modes);
	bool DelUser(const char *Nick);
	struct UserStruct *GetUser(const char *const Nick) const;
	bool RenameUser(const char *const OldNick, const char *const NewNick);
	void WipeUsers(void);
	
	inline const char *GetTopic(void) const
	{
		return this->Topic.c_str();
	}
	inline const char *GetWhoSetTopic(void) const
	{
		return this->WhoSetTopic.c_str();
	}
	inline const char *GetChannelName(void) const
	{
		return this->Channel.c_str();
	}
	inline unsigned GetWhenSetTopic(void) const
	{
		return this->WhenSetTopic;
	}
	inline std::map<std::string, struct UserStruct> &GetUserList(void) const
	{
		return const_cast<ChannelList*>(this)->UserList;
	}
	void SetTopic(const char *InTopic = NULL);
	void SetWhoSetTopic(const char *In = NULL);
	void SetWhenSetTopic(const unsigned WhenSetTopic = 0);
};


//Functions
namespace State
{
	bool DelChannel(const char *const Channel);
	class ChannelList *AddChannel(const char *const Channel);
	void ShutdownChannelList(void);
	class ChannelList *LookupChannel(const char *const ChannelName);
	char UserModes_Get_Mode2Symbol(const unsigned char Modes);
	unsigned char UserModes_Get_Symbol2Mode(const char Symbol);
	unsigned char UserModes_Get_Letter2Mode(char Letter);
}
//Globals
extern std::map<std::string, ChannelList> ChannelListCore;
#endif //__STATE_HEADER__
