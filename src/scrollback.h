/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**Controls the recording, formatting, and reporting of all scrollback.**/
#ifndef __SCROLLBACK_HEADER__
#define __SCROLLBACK_HEADER__

#include <time.h> //For time_t

///Structures.
struct ScrollbackList
{
	time_t Time; //The time the message was detected by NEXUS.
	const char *Msg; //The message data. Dynamically allocated on the heap.
	const char *Origin; //Where the message is from. Either a full mask or a channel. Dynamically allocated..
	const char *Target; //Who it was intended for, either us or a channel. Set to NULL to indicate a PM. Dynamically allocated.
	
	struct ScrollbackList *Next, *Prev;
};

///Globals.
extern struct ScrollbackList *ScrollbackCore;

///Functions
namespace Scrollback
{
	struct ScrollbackList *AddMsg(const char *Msg, const char *Origin, const char *Target, time_t Time);
	void DelMsg(struct ScrollbackList *ToDel);
	void Shutdown(void);
	void Reap(void);
}

#endif //__SCROLLBACK_HEADER__
