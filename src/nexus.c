/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**This file is where most ofthe big stuff is called from.**/
#include <stdio.h>
#include <string.h>
#include "netcore.h"
#include "nexus.h"
#include "config.h"
#include "irc.h"

//Static function prototypes
static void NEXUS_Init(void);

///And as I once cleansed the world with fire, I will destroy you, and your puny project! -Dr. Reed
static void NEXUS_Init(void)
{ //Starts networking and whatnot.
	Net_InitServer(NEXUSConfig.PortNum); //Start up the NEXUS server.

	
	NEXUS_Loop();
	
	return;
}

///This is where shit gets real.
void NEXUS_Loop(void)
{
}


int main(void)
{
	const char *const Stringy = "JOIN ##aqu4bot\r\nPRIVMSG ##aqu4bot :Hello from NEXUS HQ!\r\n";
	if (!Config_ReadConfig())
	{
		fprintf(stderr, "Failed to load configuration!\n");
		return 1;
	}
	
	IRC_Connect();
	Net_Write(IRCDescriptor, (void*)Stringy, strlen(Stringy));
	
	return 0;
}
