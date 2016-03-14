/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**Header file for config.c**/

#include <stdbool.h>
#include <time.h>

#define INTERCLIENTDELAY_DEFAULT 8
//Structures
struct _IRCConfig
{
	unsigned short PortNum;
	char Nick[64], Ident[128], RealName[256];
	char Server[1024];
	char NickServUser[64], NickServPassword[256]; //Nickserv login and whatnot.
};

struct NEXUSConfig
{
	int MaxSimulConnections;
	unsigned short PortNum;
	char ServerPassword[256];
	unsigned InterclientDelay; //In 10ths of seconds.
	bool ScrollbackEnabled; //Generally true.
	time_t ScrollbackKeepTime; //How long in seconds we keep scrollback.
};

//Globals
extern struct NEXUSConfig NEXUSConfig;
extern struct _IRCConfig IRCConfig;
extern char ConfigFilePath[1024];

//Functions
namespace Config
{
	bool ReadConfig(void);
	bool CheckConfig(void);
}
