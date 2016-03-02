/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**Loads configuration from NEXUS.conf.**/

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "config.h"
#include "nexus.h"
#define CONFIG_DIR ".nexus"
#define CONFIG_FILE "NEXUS.conf"

#define IRC_PORT_DEFAULT 6667
#define NEXUS_PORT_DEFAULT IRC_PORT_DEFAULT
#define NEXUS_MAXSIMUL_DEFAULT 256

char ConfigFilePath[1024]; //We need to get the user's home directory.

struct _IRCConfig IRCConfig = { IRC_PORT_DEFAULT };

struct NEXUSConfig NEXUSConfig = { NEXUS_MAXSIMUL_DEFAULT, NEXUS_PORT_DEFAULT,
								 { '\0' }, INTERCLIENTDELAY_DEFAULT, true,
								(60 * 60) * 8 //Eight hour default
								};

bool Config_ReadConfig(void)
{
	FILE *FD = NULL;
	struct stat FileStat;
	char *ConfigData = NULL, *Worker = NULL;
	char LineData[1024]; //Should be enough for anybody
	const char *CopyFrom = NULL; //Just handy for getting values of config
	unsigned LineNum = 1;
	
	if (!*ConfigFilePath)
	{
		//We need to determine the path.
		snprintf(ConfigFilePath, sizeof ConfigFilePath, "%s/" CONFIG_DIR "/" CONFIG_FILE, getenv("HOME"));
	}
	
	if ((FD = fopen(ConfigFilePath, "r")) == NULL || stat(ConfigFilePath, &FileStat) != 0)
	{
		return false;
	}
	
	if (FileStat.st_size == 0)
	{
		fprintf(stderr, "Configuration file is completely empty!\n");\
		return false;
	}
	
	//Allocate space.
	ConfigData = new char[FileStat.st_size + 1];
	
	//Read in the config file.
	fread(ConfigData, 1, FileStat.st_size, FD);
	ConfigData[FileStat.st_size] = '\0';
	
	//Nuke ending newlines and carriage returns.
	Worker = ConfigData + FileStat.st_size - 1;
	while (*Worker == '\n' || *Worker == '\r') *Worker-- = '\0';
	
	///Parse said config file.
	Worker = ConfigData; 
	do
	{
		char *LineWorker = LineData, *LE = LineData + sizeof LineData - 1;
		
		//Skip past newlines and carriage returns.
		while (*Worker == '\n' || *Worker == '\r') ++Worker;
		
		if (*Worker == '#') continue;//Denotes a comment in config file.
		
		//Read in this line.
		for (; *Worker != '\n' && *Worker != '\r' && *Worker != '\0' && LineWorker != LE; ++Worker, ++LineWorker)
		{
			*LineWorker = *Worker;
		}
		*LineWorker = '\0';
		
		LineWorker = LineData;
		
		///Process the line.
		
		if (!strchr(LineData, '='))
		{ //Everything should have an equals somewhere.
			fprintf(stderr, "Missing '=' in config line, line %u\n", LineNum);
			continue; //Can't do anything about it, so just go to the next line.
		}
		
		if (!strncmp(LineData, "IRC.Server=", sizeof "IRC.Server=" - 1))
		{
			CopyFrom = LineData + sizeof "IRC.Server=" - 1;
			
			if (*IRCConfig.Server) continue; //Already handed to us via CLI.
			
			strncpy(IRCConfig.Server, CopyFrom, sizeof IRCConfig.Server - 1);
			IRCConfig.Server[sizeof IRCConfig.Server - 1] = '\0';
		}
		else if (!strncmp(LineData, "IRC.Port=", sizeof "IRC.Port=" - 1))
		{
			CopyFrom = LineData + sizeof "IRC.Port=" - 1;
			
			if (IRCConfig.PortNum != IRC_PORT_DEFAULT) continue; //Already given via config file.
			
			IRCConfig.PortNum = atoi(CopyFrom);
		}
		else if (!strncmp(LineData, "IRC.Nick=", sizeof "IRC.Nick=" - 1))
		{
			CopyFrom = LineData + sizeof "IRC.Nick=" - 1;
			
			if (*IRCConfig.Nick) continue;
			
			strncpy(IRCConfig.Nick, CopyFrom, sizeof IRCConfig.Nick - 1);
			IRCConfig.Nick[sizeof IRCConfig.Nick - 1] = '\0';
		}
		else if (!strncmp(LineData, "IRC.Ident=", sizeof "IRC.Ident=" - 1))
		{
			CopyFrom = LineData + sizeof "IRC.Ident=" - 1;
			
			if (*IRCConfig.Ident) continue;
			
			strncpy(IRCConfig.Ident, CopyFrom, sizeof IRCConfig.Ident - 1);
			IRCConfig.Ident[sizeof IRCConfig.Ident - 1] = '\0';
		}
		else if (!strncmp(LineData, "IRC.RealName=", sizeof "IRC.RealName=" - 1))
		{
			CopyFrom = LineData + sizeof "IRC.RealName=" - 1;
			
			if (*IRCConfig.RealName) continue;
			
			strncpy(IRCConfig.RealName, CopyFrom, sizeof IRCConfig.RealName - 1);
			IRCConfig.RealName[sizeof IRCConfig.RealName - 1] = '\0';
		}
		else if (!strncmp(LineData, "IRC.NickServPassword=", sizeof "IRC.NickServPassword=" - 1))
		{
			CopyFrom = LineData + sizeof "IRC.NickServPassword=" - 1;
			
			strncpy(IRCConfig.NickServPassword, CopyFrom, sizeof IRCConfig.NickServPassword - 1);
			IRCConfig.NickServPassword[sizeof IRCConfig.NickServPassword - 1] = '\0';
		}
		else if (!strncmp(LineData, "IRC.NickServUser=", sizeof "IRC.NickServUser=" - 1))
		{
			CopyFrom = LineData + sizeof "IRC.NickServUser=" - 1;
			
			strncpy(IRCConfig.NickServUser, CopyFrom, sizeof IRCConfig.NickServUser - 1);
			IRCConfig.NickServUser[sizeof IRCConfig.NickServUser - 1] = '\0';
		}
		else if (!strncmp(LineData, "NEXUS.Port=", sizeof "NEXUS.Port=" - 1))
		{
			CopyFrom = LineData + sizeof "NEXUS.Port=" - 1;
			
			if (NEXUSConfig.PortNum != NEXUS_PORT_DEFAULT) continue;
			
			NEXUSConfig.PortNum = atoi(CopyFrom);
		}
		else if (!strncmp(LineData, "NEXUS.MaxSimul=", sizeof "NEXUS.MaxSimul=" - 1))
		{
			CopyFrom = LineData + sizeof "NEXUS.MaxSimul=" - 1;
			
			if (NEXUSConfig.MaxSimulConnections != NEXUS_MAXSIMUL_DEFAULT) continue;
			
			NEXUSConfig.MaxSimulConnections = atoi(CopyFrom);
		}
		else if (!strncmp(LineData, "NEXUS.NEXUSPassword=", sizeof "NEXUS.NEXUSPassword=" - 1))
		{
			CopyFrom = LineData + sizeof "NEXUS.NEXUSPassword=" - 1;
			
			if (*NEXUSConfig.ServerPassword) continue; //Password already set at cli.
			
			strncpy(NEXUSConfig.ServerPassword, CopyFrom, sizeof NEXUSConfig.ServerPassword - 1);
			NEXUSConfig.ServerPassword[sizeof NEXUSConfig.ServerPassword - 1] = '\0';
		}
		else if (!strncmp(LineData, "NEXUS.InterclientDelay=", sizeof "NEXUS.InterclientDelay" - 1))
		{
			CopyFrom = LineData + sizeof "NEXUS.InterclientDelay" - 1;
			
			if (NEXUSConfig.InterclientDelay != INTERCLIENTDELAY_DEFAULT) continue; //We already have it from CLI.
			
			NEXUSConfig.InterclientDelay = atoi(CopyFrom);
		}
		else if (!strncmp(LineData, "NEXUS.ScrollbackEnabled=", sizeof "NEXUS.ScrollbackEnabled=" - 1))
		{
			CopyFrom = LineData + sizeof "NEXUS.ScrollbackEnabled=" - 1;
			
			if (!strcmp(CopyFrom, "true")) NEXUSConfig.ScrollbackEnabled = true;
			else NEXUSConfig.ScrollbackEnabled = false;
		}
		else if (!strncmp(LineData, "NEXUS.ScrollbackKeepTime=", sizeof "NEXUS.ScrollbackKeepTime=" - 1))
		{
			CopyFrom = LineData + sizeof "NEXUS.ScrollbackKeepTime=" - 1;
			
			switch (*CopyFrom++)
			{
				case 'S':
				case 's': //Seconds.
					NEXUSConfig.ScrollbackKeepTime = atoi(CopyFrom);
					break;
				case 'M':
				case 'm':
					NEXUSConfig.ScrollbackKeepTime = atoi(CopyFrom) * 60;
					break;
				case 'H':
				case 'h':
					NEXUSConfig.ScrollbackKeepTime = atoi(CopyFrom) * 60 * 60;
					break;
				case 'D':
				case 'd':
					NEXUSConfig.ScrollbackKeepTime = atoi(CopyFrom) * ((60 * 60) * 24);
					break;
				default:
					fprintf(stderr, "Bad ScrollbackKeepTinme value, line %u in %s\n", LineNum, ConfigFilePath);
					break;
			}
		}
		else
		{
			fprintf(stderr, "Bad config value \"%s\" on line %u in \"%s\"\n", LineData, LineNum, ConfigFilePath);
			continue;
		}
	} while (++LineNum, (Worker = strpbrk(Worker, "\r\n")));

	delete[] ConfigData;
	return true;
}

bool Config_CheckConfig(void)
{
	//We need at least a nick and a server name.
	if (!*IRCConfig.Nick || !*IRCConfig.Server) return false;
	
	//No ident? Make our nick our ident.
	if (!*IRCConfig.Ident) strcpy(IRCConfig.Ident, IRCConfig.Nick);
	
	//No realname? Make our nick our realname.
	if (!*IRCConfig.RealName) strcpy(IRCConfig.RealName, IRCConfig.Nick);
	
	//all we need is a nickserv password, so if you don't give us a username, we'll use your nick.
	if (*IRCConfig.NickServPassword && !*IRCConfig.NickServUser) strcpy(IRCConfig.NickServUser, IRCConfig.Nick);
	
	return true;
}
