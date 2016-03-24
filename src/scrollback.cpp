/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**Controls the recording, formatting, and reporting of all scrollback.**/

#include "scrollback.h"
#include "config.h"
#include "nexus.h"

#include "substrings/substrings.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <list>
#include <string>

static std::list<class ScrollbackObj> ScrollbackCore;
#define SBTIMEFORMAT_DEFAULT "[%I:%M:%S %p %Y-%m-%d]"
static std::string SBTimeFormat = SBTIMEFORMAT_DEFAULT;

//Static prototypes 
static void SendPrivmsg(ScrollbackObj *Send, struct ClientListStruct *Client);
static void SendRaw(ScrollbackObj *Send, struct ClientListStruct *Client);

void Scrollback::SetTimeFormat(const char *const InFormat)
{
	
	SBTimeFormat = (InFormat && *InFormat) ? InFormat : SBTIMEFORMAT_DEFAULT;
}

void Scrollback::Shutdown(void)
{
	ScrollbackCore.clear();
}

ScrollbackObj *Scrollback::Add(const time_t Time, const enum IRCMessageType Type, const char *Msg, const char *Origin, const char *Target)
{
	ScrollbackObj New(Time, Type, Msg, Origin ? Origin : "", Target ? Target : "");
	
	ScrollbackCore.push_back(New);
	
	return &ScrollbackCore.back();
}

void Scrollback::Reap(void)
{
	//Reap expired scrollback.
	const time_t TimeCompare = time(NULL);
	
	for (std::list<class ScrollbackObj>::iterator Iter = ScrollbackCore.begin(); Iter != ScrollbackCore.end(); ++Iter)
	{
		if (Iter->GetTime() + NEXUSConfig.ScrollbackKeepTime < TimeCompare)
		{ //Expired. Reap it.
			std::list<ScrollbackObj>::iterator NewIter = Iter;
			++NewIter;
			
			ScrollbackCore.erase(Iter);
			Iter = NewIter;
			continue;
		}
	}
}

bool Scrollback::SendAllToClient(struct ClientListStruct *Client)
{
	std::list<ScrollbackObj>::iterator Iter = ScrollbackCore.begin();
	
	for (; Iter != ScrollbackCore.end(); ++Iter)
	{
		ScrollbackObj *Send = &*Iter; //Because efficiency.
		
		switch (Send->GetType())
		{
			case IRCMSG_PRIVMSG:
				SendPrivmsg(Send, Client);
				break;
			default:
				SendRaw(Send, Client);
				break;
		}
	}
	return true;
}


static void SendPrivmsg(ScrollbackObj *Send, struct ClientListStruct *Client)
{
	if (Send->GetType() != IRCMSG_PRIVMSG) return;
	
	char ScrollBuf[2048];
	
	const time_t Time = Send->GetTime();
	
	struct tm *TimeStruct = localtime(&Time);
	char TimeBuf[128];
	const char *Origin = Send->GetOrigin();
	
	strftime(TimeBuf, sizeof TimeBuf, SBTimeFormat.c_str(), TimeStruct);

	std::string SelfOrigin = std::string(IRCConfig.Nick) + '!' + Client->Ident + '@' + Client->IP;
	
	if (!*Origin)
	{ //We want to use ourselves as the origin if none specified.
		Origin = SelfOrigin.c_str();
	}
	
	if (SubStrings.StartsWith("\1ACTION ", Send->GetMsg()))
	{
		char *NewMsg = new char[strlen(Send->GetMsg()) + 1];
		
		//Don't bother trying to understand the arithmetic here.
		SubStrings.Copy(NewMsg, Send->GetMsg() + (sizeof "\1ACTION " - 1), (strlen(Send->GetMsg()) + 1) - (sizeof "\1ACTION " - 1));
		
		//Chop off the \x01 at the end.
		SubStrings.StripTrailingChars(NewMsg, "\001");
		
		//Build the message.
		char Nick[64], Ident[256], Mask[512];
		IRC::BreakdownNick(Origin, Nick, Ident, Mask);
		
		snprintf(ScrollBuf, sizeof ScrollBuf, ":%s PRIVMSG %s :\0034%s\3 \002\0033**\2\3 %s %s \002\0033**\2\3\r\n",
				Origin, (Send->GetTarget() ? Send->GetTarget() : IRCConfig.Nick), TimeBuf, Nick, NewMsg);
		delete[] NewMsg;
	}
	else
	{
		snprintf(ScrollBuf, sizeof ScrollBuf, ":%s PRIVMSG %s :\0034%s\3 %s\r\n",
			Origin, (Send->GetTarget() ? Send->GetTarget() : IRCConfig.Nick), TimeBuf, Send->GetMsg());
	}
	
	//Now send it to the client.
	Client->SendLine(ScrollBuf);
}

static void SendRaw(ScrollbackObj *Send, struct ClientListStruct *Client)
{
	Client->SendLine(Send->GetMsg());
}
