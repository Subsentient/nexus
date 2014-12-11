/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**This file is where most of the big stuff is called from. main() is also in here.**/
#include <stdio.h>
#include <string.h>
#include "netcore.h"
#include "nexus.h"
#include "config.h"
#include "irc.h"
#include "server.h"

int main(void)
{ ///And as I once cleansed the world with fire, I will destroy you, and your puny project! -Dr. Reed

	//Load NEXUS.conf and whatnot
	printf("Reading configuration... ");
	if (!Config_ReadConfig())
	{
		fprintf(stderr, "Failed to load configuration!\n");
		return 1;
	}
	puts("Done.");
	
	//Connect to the REAL IRC server.
	printf("Connecting to IRC server \"%s:%hu\"... ", IRCConfig.Server, IRCConfig.PortNum);
	if (!IRC_Connect())
	{
		fprintf(stderr, "Unable to connect to IRC server!\n");
		return 1;
	}
	puts("Done.");
	
	
	//Bring up the NEXUS pseudo-IRC-server.
	printf("Bringing up NEXUS server on port %hu... ", NEXUSConfig.PortNum);
	if (!Net_InitServer(NEXUSConfig.PortNum))
	{
		fprintf(stderr, "Failed to bring up NEXUS server on port %hu.\n", NEXUSConfig.PortNum);
		return 1;
	}
	puts("Done.");
	
	while(1) Server_Loop();
	
	return 0;
}
