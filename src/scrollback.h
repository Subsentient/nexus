/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**Controls the recording, formatting, and reporting of all scrollback.**/
#ifndef __SCROLLBACK_HEADER__
#define __SCROLLBACK_HEADER__

#include <time.h> //For time_t

#include "irc.h" //For enum IRCMessageType

///Structures.
class ScrollbackObj
{
	time_t Time; //The time the message was detected by NEXUS.
	enum IRCMessageType Type;
	std::string Msg; //The message data. Dynamically allocated on the heap.
	std::string Origin; //Where the message is from. Either a full mask or a channel.
	std::string Target; //Who it was intended for, either us or a channel. Keep empty to indicate a PM.
public:
	inline time_t GetTime(void) const
	{
		return this->Time;
	}
	inline enum IRCMessageType GetType(void) const
	{
		return this->Type;
	}
	inline const char *GetMsg(void) const
	{
		return this->Msg.c_str();
	}
	inline const char *GetOrigin(void) const
	{
		return this->Origin.c_str();
	}
	inline const char *GetTarget(void) const
	{
		return this->Target.c_str();
	}
	ScrollbackObj(const time_t InTime, const enum IRCMessageType InType, const char *InMsg, const char *InOrigin = "", const char *InTarget = "")
					: Time(InTime), Type(InType), Msg(InMsg), Origin(InOrigin), Target(InTarget)
	{
	}
	ScrollbackObj(void) : Time(0), Type(IRCMSG_INVALID) {}
};

///Globals.

///Functions
namespace Scrollback
{
	void SetTimeFormat(const char *const InFormat = NULL);
	ScrollbackObj *Add(const time_t Time, const enum IRCMessageType Type, const char *Msg = NULL, const char *Origin = NULL, const char *Target = NULL);
	bool SendAllToClient(struct ClientListStruct *Client);
	void Shutdown(void);
}

#endif //__SCROLLBACK_HEADER__
