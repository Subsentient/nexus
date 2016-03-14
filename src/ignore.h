/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/
#include <limits.h>


#define NEXUS_IGNORE_CHANMSG (1U) //Messages in a channel
#define NEXUS_IGNORE_PRIVMSG (1U << 1) //PMs
#define NEXUS_IGNORE_NOTICE (1U << 2) //Notices
#define NEXUS_IGNORE_VISIBLE (1U << 3) //Notices
#define NEXUS_IGNORE_ALL (NEXUS_IGNORE_CHANMSG | NEXUS_IGNORE_PRIVMSG | NEXUS_IGNORE_NOTICE | NEXUS_IGNORE_VISIBLE)//Everything.


//Structs
struct IgnoreList
{
	char Nick[64], Ident[64], Mask[256];
	unsigned WhatToBlock;
	
	struct IgnoreList *Next, *Prev;
};


//Functions
namespace Ignore
{
	bool Check(const char *Message, const unsigned WhatToCheck);
	bool Check_Separate(const char *const Nick, const char *const Ident, const char *const Mask, const unsigned WhatToCheck);
	void Shutdown(void);
	bool Modify(const char *const VHost, const bool Adding, const unsigned WhatToChange);
	bool Add(const char *Message, const unsigned WhatToBlock);
}
	
