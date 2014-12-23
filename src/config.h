/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**Header file for config.c**/

#include <stdbool.h>

#define INTERCLIENTDELAY_DEFAULT 8
//Structures
struct IRCConfig
{
	char Nick[64], Ident[128], RealName[256];
	char Server[1024];
	char NickServUser[64], NickServPassword[256]; //Nickserv login and whatnot.
	unsigned short PortNum;
};

struct NEXUSConfig
{
	int MaxSimulConnections;
	unsigned short PortNum;
	char ServerPassword[256];
	unsigned InterclientDelay; //In 10ths of seconds.
};

//Globals
extern struct NEXUSConfig NEXUSConfig;
extern struct IRCConfig IRCConfig;
extern char ConfigFilePath[1024];

//Functions
bool Config_ReadConfig(void);
bool Config_CheckConfig(void);
