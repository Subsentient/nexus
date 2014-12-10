/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**Loads configuration from NEXUS.conf.**/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "config.h"

#define CONFIGDIR "/geekhut/.nexus/"
#define CONFIGFILE "NEXUS.conf"

struct IRCConfig IRCConfig;

struct NEXUSConfig NEXUSConfig = { .MaxSimulConnections = 256, .PortNum = 6667 };

bool Config_ReadConfig(void)
{
	FILE *FD = fopen(CONFIGDIR CONFIGFILE, "rb");
	struct stat FileStat;
	char *ConfigData = NULL, *Worker = NULL;
	char LineData[1024]; //Should be enough for anybody
	const char *CopyFrom = NULL; //Just handy for getting values of config
	unsigned LineNum = 1;
	
	if (FD == NULL || stat(CONFIGDIR CONFIGFILE, &FileStat) != 0)
	{
		return false;
	}
	
	if (FileStat.st_size == 0)
	{
		fprintf(stderr, "Configuration file is completely empty!\n");\
		return false;
	}
	
	//Allocate space.
	ConfigData = malloc(FileStat.st_size + 1);
	
	//Read in the config file.
	fread(ConfigData, 1, FileStat.st_size, FD);
	ConfigData[FileStat.st_size] = '\0';
	
	//Nuke ending newlines and carriage returns.
	Worker = ConfigData + FileStat.st_size - 1;
	while (*Worker == '\n' || *Worker == '\r') *Worker = '\0';
	
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
			
			strncpy(IRCConfig.Server, CopyFrom, sizeof IRCConfig.Server - 1);
			IRCConfig.Server[sizeof IRCConfig.Server - 1] = '\0';
		}
		else if (!strncmp(LineData, "IRC.Port=", sizeof "IRC.Port=" - 1))
		{
			CopyFrom = LineData + sizeof "IRC.Port=" - 1;
			
			IRCConfig.PortNum = atoi(CopyFrom);
		}
		else if (!strncmp(LineData, "IRC.Nick=", sizeof "IRC.Nick=" - 1))
		{
			CopyFrom = LineData + sizeof "IRC.Nick=" - 1;
			
			strncpy(IRCConfig.Nick, CopyFrom, sizeof IRCConfig.Nick - 1);
			IRCConfig.Nick[sizeof IRCConfig.Nick - 1] = '\0';
		}
		else if (!strncmp(LineData, "IRC.Ident=", sizeof "IRC.Ident=" - 1))
		{
			CopyFrom = LineData + sizeof "IRC.Ident=" - 1;
			
			strncpy(IRCConfig.Ident, CopyFrom, sizeof IRCConfig.Ident - 1);
			IRCConfig.Ident[sizeof IRCConfig.Ident - 1] = '\0';
		}
		else if (!strncmp(LineData, "IRC.RealName=", sizeof "IRC.RealName=" - 1))
		{
			CopyFrom = LineData + sizeof "IRC.RealName=" - 1;
			
			strncpy(IRCConfig.RealName, CopyFrom, sizeof IRCConfig.RealName - 1);
			IRCConfig.RealName[sizeof IRCConfig.RealName - 1] = '\0';
		}
		else if (!strncmp(LineData, "NEXUS.Port=", sizeof "NEXUS.Port=" - 1))
		{
			CopyFrom = LineData + sizeof "NEXUS.Port=" - 1;
			
			NEXUSConfig.PortNum = atoi(CopyFrom);
		}
		else if (!strncmp(LineData, "NEXUS.MaxSimul=", sizeof "NEXUS.MaxSimul=" - 1))
		{
			CopyFrom = LineData + sizeof "NEXUS.MaxSimul=" - 1;
			
			NEXUSConfig.MaxSimulConnections = atoi(CopyFrom);
		}
		else
		{
			fprintf(stderr, "Bad config value \"%s\" on line %u in \"" CONFIGDIR CONFIGFILE "\"\n", LineData, LineNum);
			continue;
		}
	} while (++LineNum, (Worker = strpbrk(Worker, "\r\n")));
	
	return true;
}

