/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**Main header file for NEXUS.**/

#ifndef __NEXUS_HEADER__
#define __NEXUS_HEADER__

#ifdef DEBUG
#define Exit(Code) printf("Exit code %s at %s:%d\n", #Code, __FILE__, __LINE__); exit(Code)
#else
#define Exit exit
#endif //DEBUG

#include <stddef.h>
#include "server.h"

#define NEXUS_VERSION "0.1"
#define CONTROL_NICKNAME "NEXUS_CONTROL"
namespace NEXUS
{
	void IRC2NEXUS(const char *Message);
	void NEXUS2IRC(const char *Message, struct ClientListStruct *const Client);
	void DescriptorSet_Add(const int Descriptor);
	bool DescriptorSet_Del(const int Descriptor);
	void ProcessIdleActions(void);
	void CleanTerminate(const int ExitCode, const char *const QuitReason = "No reason provided");
}

namespace Util
{
	inline bool IsChannelSymbol(const char Sym)
	{
		switch (Sym)
		{
			case '#':
			case '&':
				return true;
				break;
			default:
				return false;
				break;
		}
	}
}
#endif //__NEXUS_HEADER__
