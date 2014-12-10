/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**Header file for config.c**/

#include <stdbool.h>

//Structures
struct IRCConfig
{
	char Nick[64], Ident[128], RealName[256];
	char Server[1024];
	unsigned short PortNum;
};

struct NEXUSConfig
{
	int MaxSimulConnections;
	unsigned short PortNum;
};

//Globals
extern struct NEXUSConfig NEXUSConfig;
extern struct IRCConfig IRCConfig;


//Functions
bool Config_ReadConfig(void);
