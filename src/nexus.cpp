/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**This file is where most of the big stuff is called from. main() is also in here.**/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#ifdef WIN
#include <winsock2.h>
#endif //WIN

#include "substrings/substrings.h"
#include "netcore.h"
#include "nexus.h"
#include "config.h"
#include "irc.h"
#include "server.h"
#include "state.h"
#include "scrollback.h"
#include "ignore.h"

//Prototypes

//Globals
static fd_set DescriptorSet;
static int DescriptorMax;

namespace NEXUS
{
	static void HandleClientInterface(const char *const Message, struct ClientListStruct *Client);
}

void NEXUS::ProcessIdleActions(void)
{
	std::list<struct ClientListStruct>::iterator Iter = ClientListCore.begin();
	
	for (; Iter != ClientListCore.end(); ++Iter)
	{
		struct ClientListStruct *Client = &*Iter;
		
		if (!Client->WaitingForPing)
		{
			if (Client->PingSentTime + NEXUSConfig.ClientPingInterval <= time(NULL))
			{ //It's time to ping them. Don't send them data until we get a response.
				Client->Ping();
				continue;
			}
			else
			{ //They are up-to-date on pings, send them all data we have queued for them.
				if (!Client->WriteQueue.empty() && !Client->FlushSendBuffer())
				{ //Error sending them their data. Time to let them die.
					std::list<struct ClientListStruct>::iterator NewIter = Iter;
					++NewIter;
					
					Net::Close(Client->Descriptor);
					NEXUS::DescriptorSet_Del(Client->Descriptor);
					Server::ClientList::Del(Client->Descriptor);
					
					Iter = NewIter;
				}
			}
		}
		else
		{ //We need to check if they have pinged out.
			if (Client->PingSentTime + NEXUSConfig.ClientPingoutTime <= time(NULL))
			{ //They have, it's time to say byebye.
				std::list<struct ClientListStruct>::iterator NewIter = Iter;
				++NewIter;
				
				Net::Close(Client->Descriptor);
				NEXUS::DescriptorSet_Del(Client->Descriptor);
				Server::ClientList::Del(Client->Descriptor);
				
				Iter = NewIter;
				continue;
			}
		}
	}
}

void NEXUS::MasterLoop(void)
{
	while (1)
	{
		fd_set Set = DescriptorSet;
		fd_set ErrSet = Set;
		
		struct timeval Timeout = { 0, 50000 }; //Microseconds. 1 million = 1 sec
		
		//select() gives you your results in Set, which is also what it initially reads from.		
		switch (select(DescriptorMax + 1, &Set, NULL, &ErrSet, &Timeout))
		{
			case -1:
				fputs("Failure with select()\n", stderr);
				Exit(1);
				break;
			case 0:
			{ //Nobody wants to talk to us, so we just do our own thing.
				goto HandleIdleStuff;
			}
			default:
				break;
		}

		int Inc;
		
		for (Inc = 0; Inc <= DescriptorMax; ++Inc)
		{
			//Process errors.
			if (FD_ISSET(Inc, &ErrSet))
			{
				if (Inc == IRCDescriptor)
				{
					goto Errorrrz; //basically says we ded
				}
				
				
				struct ClientListStruct *Client = Server::ClientList::Lookup(Inc);
				
				//Not a whole lot we can do.
				if (!Client) continue;
				
				Net::Close(Client->Descriptor);
				NEXUS::DescriptorSet_Del(Client->Descriptor);
				Server::ClientList::Del(Client->Descriptor);
				
				//Remove it from the 'read' set if it's there.
				if (FD_ISSET(Inc, &Set))
				{
					FD_CLR(Inc, &Set);
					if (Inc == DescriptorMax) --DescriptorMax;
				}
				
				continue;
			}
			
			if (!FD_ISSET(Inc, &Set)) continue;
			
			if (Inc == ServerDescriptor)
			{ //A client wants to join us.
				struct ClientListStruct *const Client = Server::AcceptLoop();
				if (!Client) continue;
				
				NEXUS::DescriptorSet_Add(Client->Descriptor);
				continue;
			}
			else if (Inc == IRCDescriptor)
			{ //Something from IRC.
				char IRCBuf[2048];

				//Check IRC for data.
				bool NRR;
				NRR = Net::Read(IRCDescriptor, IRCBuf, sizeof IRCBuf, true);
			
				if (!NRR)
				{ //Error.
					char OutBuf[2048];
				Errorrrz:
					IRC::Disconnect();
					
					//Tell everyone what happened.
					snprintf(OutBuf, sizeof OutBuf, ":" CONTROL_NICKNAME "!NEXUS@NEXUS NOTICE %s :NEXUS has lost the connection to %s:%hu and is shutting down.\r\n", IRCConfig.Nick, IRCConfig.Server, IRCConfig.PortNum);		
					Server::ForwardToAll(OutBuf);
					
					Server::SendQuit(-1, "NEXUS has lost the connection to the IRC server."); //Now make them quit.
					Net::ShutdownServer();
					
					Exit(1);
				}
				
				//Process the data.
				IRC::Loop(IRCBuf);
				continue;
			}
			else
			{ //Client wants something.
				struct ClientListStruct *Client = Server::ClientList::Lookup(Inc);
				
				if (!Client)
				{
					//Kill their connection and remove them from the descriptor set.
					Net::Close(Inc);
					NEXUS::DescriptorSet_Del(Inc);
					continue; //BS client
				}
				
				char ClientBuf[2048];
				
				const bool NRR = Net::Read(Client->Descriptor, ClientBuf, sizeof ClientBuf, true);
				
				
				if (!NRR)
				{ //he ded
					Net::Close(Client->Descriptor);
					NEXUS::DescriptorSet_Del(Client->Descriptor);
					Server::ClientList::Del(Client->Descriptor);
					continue;
				}
				
				NEXUS::NEXUS2IRC(ClientBuf, Client);
				continue;
			}

		}

	HandleIdleStuff:
		//We're done processing any descriptors, so now we handle anything we should do in our spare time.
		NEXUS::ProcessIdleActions();
	}
}

void NEXUS::DescriptorSet_Add(const int Descriptor)
{
	if (FD_ISSET(Descriptor, &DescriptorSet)) return;
	
	if (Descriptor > DescriptorMax) DescriptorMax = Descriptor;
	
	FD_SET(Descriptor, &DescriptorSet);
}

bool NEXUS::DescriptorSet_Del(const int Descriptor)
{
	if (!FD_ISSET(Descriptor, &DescriptorSet)) return false;
	
	if (Descriptor == DescriptorMax)
	{
		int Dec = DescriptorMax;
		
		for (; !FD_ISSET(Dec, &DescriptorSet); --Dec);
		
		DescriptorMax = Dec;
	}
	
	FD_CLR(Descriptor, &DescriptorSet);
	return true;
}


	
//Functions;
int main(int argc, char **argv)
{ ///And as I once cleansed the world with fire, I will destroy you, and your puny project! -Dr. Reed
	//Print version and whatnot.

	//Turn off buffering. We hates it! -Gollum
	setvbuf(stdout, NULL, _IONBF, 0); //Really, all buffering serves to do for us is mess up our console output. No thanks.
	setvbuf(stderr, NULL, _IONBF, 0);
	
	
	//Ignore sigpipe from send() in Net::Write()
	signal(SIGPIPE, SIG_IGN);
	
	puts("NEXUS BNC " NEXUS_VERSION "\n");
#ifndef WIN
	bool Background = false;
#endif
	///Process command line arguments.
	if (argc > 1)
	{ //Command line arguments override any values in configuration files.
		int Inc = 1;
		const char *ArgData = NULL;
		
		for (; Inc < argc; ++Inc)
		{
			if (!strcmp("--help", argv[Inc]))
			{
				printf("Available options:\n\n"
						"--configfile=\n"
#ifndef WIN
						"--background\n"
#endif //WIN
						"--ircserver=myserver.com\n"
						"--ircport=6667\n"
						"--ircnick=mynickname\n"
						"--ircident=myidentname\n"
						"--ircrealname=myrealname\n"
						"--ircnickservuser=username\n"
						"--ircnickservpassword=password\n"
						"--maxsimulclient=1024\n"
						"--nexusport=6667\n"
						"--nexuspassword=password\n"
						"--scrollbackenabled=true/false\n"
						"--scrollbackkeeptime=(time)\n");
				Exit(0);
			}
			else if (!strncmp(argv[Inc], "--configfile=", sizeof "--configfile=" - 1))
			{ //Allow specifying a config file location.
				ArgData = argv[Inc] + sizeof "--configfile=" - 1;
				
				strncpy(ConfigFilePath, ArgData, sizeof ConfigFilePath - 1);
				ConfigFilePath[sizeof ConfigFilePath - 1] = '\0';
			}
			else if (!strncmp(argv[Inc], "--ircserver=", sizeof "--ircserver" - 1))
			{ //IRC server hostname.
				ArgData = argv[Inc] + sizeof "--ircserver=" - 1;
				if (!*ArgData) continue;

				strncpy(IRCConfig.Server, ArgData, sizeof IRCConfig.Server - 1);
				IRCConfig.Server[sizeof IRCConfig.Server - 1] = '\0';
			}
			else if (!strncmp(argv[Inc], "--ircport=", sizeof "--ircport=" - 1))
			{ //IRC port number.
				ArgData = argv[Inc] + sizeof "--ircport=" - 1;
				if (!*ArgData) continue;

				IRCConfig.PortNum = atoi(ArgData);
			}
			else if (!strncmp(argv[Inc], "--ircnick=", sizeof "--ircnick=" - 1))
			{
				ArgData = argv[Inc] + sizeof "--ircnick=" - 1;
				if (!*ArgData) continue;

				strncpy(IRCConfig.Nick, ArgData, sizeof IRCConfig.Nick - 1);
				IRCConfig.Nick[sizeof IRCConfig.Nick - 1] = '\0';
			}
			else if (!strncmp(argv[Inc], "--ircident=", sizeof "--ircident=" - 1))
			{
				ArgData = argv[Inc] + sizeof "--ircident=" - 1;
				if (!*ArgData) continue;

				strncpy(IRCConfig.Ident, ArgData, sizeof IRCConfig.Ident - 1);
				IRCConfig.Ident[sizeof IRCConfig.Ident - 1] = '\0';
			}
			else if (!strncmp(argv[Inc], "--ircrealname=", sizeof "--ircrealname=" - 1))
			{
				ArgData = argv[Inc] + sizeof "--ircrealname=" - 1;
				if (!*ArgData) continue;

				strncpy(IRCConfig.RealName, ArgData, sizeof IRCConfig.RealName - 1);
				IRCConfig.RealName[sizeof IRCConfig.RealName - 1] = '\0';
			}
			else if (!strncmp(argv[Inc], "--ircnickservuser=", sizeof "--ircnickservuser=" - 1))
			{
				ArgData = argv[Inc] + sizeof "--ircnickservuser=" - 1;
				
				if (!*ArgData) continue;
				
				strncpy(IRCConfig.NickServUser, ArgData, sizeof IRCConfig.NickServUser - 1);
				IRCConfig.NickServUser[sizeof IRCConfig.NickServUser - 1] = '\0';
			}
			else if (!strncmp(argv[Inc], "--ircnickservpassword=", sizeof "--ircnickservpassword=" - 1))
			{
				ArgData = argv[Inc] + sizeof "--ircnickservpassword=" - 1;
				
				if (!*ArgData) continue;
				
				strncpy(IRCConfig.NickServPassword, ArgData, sizeof IRCConfig.NickServPassword - 1);
				IRCConfig.NickServPassword[sizeof IRCConfig.NickServPassword - 1] = '\0';
			}
			else if (!strncmp(argv[Inc], "--maxsimulclients=", sizeof "--maxsimulclients=" - 1))
			{
				ArgData = argv[Inc] + sizeof "--maxsimulclients=" - 1;
				if (!*ArgData) continue;

				NEXUSConfig.MaxSimulConnections = atoi(ArgData);
			}
			else if (!strncmp(argv[Inc], "--nexusport=", sizeof "--nexusport=" - 1))
			{
				ArgData = argv[Inc] + sizeof "--nexusport=" - 1;
				if (!*ArgData) continue;
				
				NEXUSConfig.PortNum = atoi(ArgData);
			}
			else if (!strncmp(argv[Inc], "--nexuspassword=", sizeof "--nexuspassword=" - 1))
			{
				ArgData = argv[Inc] + sizeof "--nexuspassword=" - 1;
				if (!*ArgData) continue;
				
				strncpy(NEXUSConfig.ServerPassword, ArgData, sizeof NEXUSConfig.ServerPassword - 1);
				NEXUSConfig.ServerPassword[sizeof NEXUSConfig.ServerPassword - 1] = '\0';
			}
#ifndef WIN
			else if (!strcmp(argv[Inc], "--background"))
			{
				puts("NEXUS set to background after config load.");
				Background = true;
			}
#endif
			else if (!strncmp(argv[Inc], "--scrollbackenabled=", sizeof "--scrollbackenabled=" - 1))
			{
				ArgData = argv[Inc] + sizeof "--scrollbackenabled=" - 1;
				
				if (!strcmp(ArgData, "true")) NEXUSConfig.ScrollbackEnabled = true;
				else NEXUSConfig.ScrollbackEnabled = false;
			}
			else if (!strncmp(argv[Inc], "--scrollbackkeeptime=", sizeof "--scrollbackkeeptime=" - 1))
			{
				ArgData = argv[Inc] + sizeof "--scrollbackkeeptime=" - 1;
				
				switch (*ArgData++)
				{
					case 's': //Seconds.
					case 'S':
						NEXUSConfig.ScrollbackKeepTime = atoi(ArgData);
						break;
					case 'm':
					case 'M':
						NEXUSConfig.ScrollbackKeepTime = atoi(ArgData) * 60;
						break;
					case 'H':
					case 'h':
						NEXUSConfig.ScrollbackKeepTime = atoi(ArgData) * 60 * 60;
						break;
					case 'D':
					case 'd':
						NEXUSConfig.ScrollbackKeepTime = atoi(ArgData) * ((60 * 60) * 24);
						break;
					default:
						fputs("Bad value for --scrollbackkeeptime.", stderr);
						break;
				}
			}
			else
			{
				fprintf(stderr, "Bad comand line option \"%s\". See --help for a list of options.\n", argv[Inc]);
				Exit(1);
			}
		}
	}
	
	if (!Config::ReadConfig())
	{ //We might still get what we need from CLI arguments.
		fprintf(stderr, "WARNING: Failed to load config file!\n");
	}
	
	if (!Config::CheckConfig())
	{ //Bad configuration.
		fprintf(stderr, "Configuration for NEXUS is invalid.\n"
				"Please check your configuration and/or command line arguments.\n");
		return 1;
	}

#ifndef WIN
	if (Background)
	{
		pid_t NewPID;
		
		puts("Backgrounding.");
		
		if ((NewPID = fork()) == -1)
		{
			fprintf(stderr, "Failed to fork()!");
			Exit(1);
		}
		else if (NewPID > 0)
		{
			signal(SIGCHLD, SIG_IGN);
			Exit(0);
		}
		else
		{
			setsid();
		}
	}
#endif //WIN

#ifdef WIN //Bring up winsock.
	WSADATA WSAData;

    if (WSAStartup(MAKEWORD(1,1), &WSAData) != 0)
    { /*Initialize winsock*/
        fprintf(stderr, "Unable to initialize WinSock2!");
        Exit(1);
    }
#endif //WIN
	
	//Connect to the REAL IRC server.
	printf("Connecting to IRC server \"%s:%hu\"... ", IRCConfig.Server, IRCConfig.PortNum);
	if (!IRC::Connect())
	{
		fprintf(stderr, "Unable to connect to IRC server!\n");
		return 1;
	}
	puts("Done.");
	
	//Track the IRC descriptor.
	NEXUS::DescriptorSet_Add(IRCDescriptor);
	
	//Bring up the NEXUS pseudo-IRC-server.
	printf("Bringing up NEXUS server on port %hu... ", NEXUSConfig.PortNum);
	if (!Net::InitServer(NEXUSConfig.PortNum))
	{
		fprintf(stderr, "Failed to bring up NEXUS server on port %hu.\n", NEXUSConfig.PortNum);
		return 1;
	}
	puts("Done.");
	
	//Track the NEXUS server descriptor
	NEXUS::DescriptorSet_Add(ServerDescriptor);

	NEXUS::MasterLoop();
	return 0;
}

/*Processes data from a client and forwards it to the IRC server.*/
void NEXUS::NEXUS2IRC(const char *Message, struct ClientListStruct *const Client)
{
	const enum ServerMessageType MsgType = Server::GetMessageType(Message);
	
	CurrentClient = Client;
	
	
	switch (MsgType)
	{
		char OutBuf[2048];
		
		default: //Nothing we care about.
		{
		ForwardVerbatim:
		
			if (!*Message)
			{ //Ded client.
				Net::Close(Client->Descriptor);
				NEXUS::DescriptorSet_Del(Client->Descriptor);
				Server::ClientList::Del(Client->Descriptor);
				return;
			}
			//Append \r\n and send to IRC server.
			snprintf(OutBuf, sizeof OutBuf, "%s\r\n", Message);
			Net::Write(IRCDescriptor, OutBuf, strlen(OutBuf));
			
			break;
		}
		case SERVERMSG_PONG:
		{ //Client responded to our pings.
			Client->CompletePing();
			break;
		}
		case SERVERMSG_PRIVMSG:
		{ //Some clients don't group our own speech correctly for PMs, so do this for channels only.
			const char *Search = strstr(Message, "PRIVMSG ");
			char Target[64], Compare[64] = CONTROL_NICKNAME;
			const char *PassOn = NULL;
			unsigned Inc = 0;
			
			if (!Search) break; //WTF?
			
			Search += sizeof "PRIVMSG " - 1;
		
			///Check if this is a communication to NEXUS.
			for (; Search[Inc] != ' ' && Search[Inc] != '\0' && Inc < sizeof Target - 1; ++Inc)
			{
				Target[Inc] = tolower(Search[Inc]);
			}
			Target[Inc] = '\0';
			
			PassOn = Search + Inc;
			
			//Lowercase our control nickname for the compare.
			for (Inc = 0; (Compare[Inc] = tolower(Compare[Inc])); ++Inc);
		
			//Now check if this is a communication to NEXUS.
			if (!strcmp(Target, Compare)) 
			{///It's for NEXUS!

				while (*PassOn == ' ' || *PassOn == ':') ++PassOn;
				
				NEXUS::HandleClientInterface(PassOn, Client);
				break; //Since it was for NEXUS, nobody else needs to see it.
			}
			
			//Don't do this for PMs. Just Channels.
			if (*Search != '#') goto ForwardVerbatim;
			
			//We need to make all text reach the other client.
			std::list<struct ClientListStruct>::iterator Iter = ClientListCore.begin();
			for (; Iter != ClientListCore.end(); ++Iter)
			{
				if (&*Iter == Client) continue; //Don't send to the one who just sent this.
				
				snprintf(OutBuf, sizeof OutBuf, ":%s!%s@%s %s\r\n", IRCConfig.Nick, Client->Ident, Client->IP, Message);
				Iter->SendLine(OutBuf);
			}
			
			//Now we want to add it to our scrollback.
			Scrollback::AddMsg(strchr(Search, ':') + 1, NULL, Target, time(NULL));
			
			goto ForwardVerbatim;
		}
		case SERVERMSG_WHO:
		{ //Only allow one client to spam WHO.
			if (Client != &*ClientListCore.begin()) break;
			else
			{
				//Turn off throttling here because it can bog things up pretty bad.
				CurrentClient = NULL;
				
				goto ForwardVerbatim;
			}
		}
		case SERVERMSG_JOIN:
		{ //Don't allow us to send joins for channels we are already in.
			char *Tempstream = (char*)malloc(strlen(Message) + 1);
			char *Worker = Tempstream + (sizeof "JOIN" - 1);
			char Channel[128];
			unsigned Inc = 0;
			unsigned OutChannelsSize = (strlen(Message) + 1) + 1024;
			char *OutChannels = (char*)malloc(OutChannelsSize);
			bool OneToJoin = false;
			
			//Turn off throttling of client out messages here because we do it for them.
			CurrentClient = NULL;
			
			SubStrings.Copy(OutChannels, "JOIN ", OutChannelsSize);
			
			memcpy(Tempstream, Message, strlen(Message) + 1);
			
			if (!Worker) break; //Corrupted.
			
			while (*Worker == ' ') ++Worker;
			
			//They sometimes send weird afterlists for their channels. If we find a space now, kill it.
			if (strchr(Worker, ' ')) *strchr(Worker, ' ') = '\0';
			
			
			///Clients are able to send lists of JOINs with a , separating them. Handle that.
			do
			{
				if (*Worker == ',') ++Worker;
				
				for (Inc = 0; Worker[Inc] != ',' && Worker[Inc] != ' ' &&
					Worker[Inc] != '\0' && Inc < sizeof Channel - 1; ++Inc)
				{ //Get the channel.
					Channel[Inc] = Worker[Inc];
				}
				Channel[Inc] = '\0';
				
				if (State::LookupChannel(Channel) == NULL)
				{ //Only forward it if we are not already in that channel.
					OneToJoin = true;
					SubStrings.Cat(OutChannels, Channel, OutChannelsSize);
					SubStrings.Cat(OutChannels, ",", OutChannelsSize);
				}
			} while ((Worker = strchr(Worker, ',')));
			
			if (OneToJoin)
			{ //We have some that are still good.
				OutChannels[strlen(OutChannels) - 1] = '\0'; //Kill comma at the end.
				SubStrings.Cat(OutChannels, "\r\n", OutChannelsSize);
				Net::Write(IRCDescriptor, OutChannels, strlen(OutChannels));
			}
			
			free(OutChannels); OutChannels = NULL;
			free(Tempstream); Tempstream = NULL;
			
			break;
		}
		case SERVERMSG_PING:
		{ //They're doing a lag check. Give them OUR response.
			char *Start = (char*)strchr(Message, ':');
			
			if (!Start) break;
			
			snprintf(OutBuf, sizeof OutBuf, ":" NEXUS_FAKEHOST " PONG " NEXUS_FAKEHOST " %s\r\n", Start);
			
			Client->SendLine(OutBuf);
			break;
		}
		case SERVERMSG_QUIT:
		{
			Server::SendQuit(Client->Descriptor, "You have sent a QUIT command to NEXUS.");
			Net::Close(Client->Descriptor);
			NEXUS::DescriptorSet_Del(Client->Descriptor);
			Server::ClientList::Del(Client->Descriptor);
			break;
		}
	}
	
	//This is necessary!
	PreviousClient = Client;
	CurrentClient = NULL; //Don't remove this either.
}

/*Processes data from the real IRC server and then sends it to functions that reformat
 * said data so that clients connected to NEXUS IRC won't get confused.*/
void NEXUS::IRC2NEXUS(const char *Message)
{
	const enum IRCMessageType MsgType = IRC::GetMessageType(Message);
	
	switch (MsgType)
	{
		char OutBuf[2048];
		
		case IRCMSG_NOTICE:
		{
			if (!Ignore::Check(Message, NEXUS_IGNORE_NOTICE))
			{ //It's not ignored, so we should forward it verbatim.
				goto ForwardVerbatim;
			}
			break;
		}
		default: //Nothing we explicitly have to deal with
		{ //Append a \r\n and then send it to everyone.
		ForwardVerbatim:
			snprintf(OutBuf, sizeof OutBuf, "%s\r\n", Message);
			Server::ForwardToAll(OutBuf);
			
			break;
		}
		case IRCMSG_PRIVMSG:
		{ //We just want to put it in the scrollback, then we send it along.
			unsigned Inc = 0;
			char Origin[256], Target[256] = { '\0' };
			const char *Worker = Message;
			
			++Worker; //Skip past the ':' at start.
			
			for (; *Worker != ' ' && *Worker != '\0' && Inc < sizeof Origin - 1; ++Inc, ++Worker)
			{ //Get the user who sent it.
				Origin[Inc] = *Worker;
			}
			Origin[Inc] = '\0';
			
			while (*Worker == ' ') ++Worker;
			
			//Go to the target of the message.
			if (!(Worker = strstr(Worker, "PRIVMSG "))) return; //Damaged data.
			Worker += strlen("PRIVMSG ");
			
			//Copy in the target.
			for (Inc = 0; *Worker != ' ' && *Worker != '\0' && Inc < sizeof Target - 1; ++Inc, ++Worker)
			{
				Target[Inc] = *Worker;
			}
			Target[Inc] = '\0';
			
			//Check if we want to ignore it
			const unsigned ToCheck = *Target == '#' ? NEXUS_IGNORE_CHANMSG : NEXUS_IGNORE_PRIVMSG;
			
			//Ignore it.
			if (Ignore::Check(Origin, ToCheck)) break;
			
			//Go to the data.
			while (*Worker == ' ') ++Worker;
			if (*Worker == ':') ++Worker;
			
			//We have data, add it to the scrollback.
			Scrollback::AddMsg(Worker, Origin, *Target == '#' ? Target : NULL, time(NULL));
			
			goto ForwardVerbatim;
		}
		case IRCMSG_TOPIC: //Either the server sent us a topic or someone has changed it.
		{
			unsigned Inc = 0;
			char NewTopic[1024];
			char Channel[256], *Worker = strchr((char*)Message, '#');
			class ChannelList *ChannelStruct = NULL;
			
			if (!Worker) return; //Corrupt data.
			
			//Get the channel name.
			for (; Worker[Inc] != ' ' && Worker[Inc] != '\0' && Inc < sizeof Channel - 1; ++Inc)
			{
				Channel[Inc] = Worker[Inc];
			}
			Channel[Inc] = '\0';
			
			Worker += Inc; //Skip past the space.
			
			if (*Worker++ == '\0') return; //Corrupt data.
			
			if (*Worker == ':') ++Worker;
			
			//Get the new topic.
			strncpy(NewTopic, Worker, sizeof NewTopic - 1);
			NewTopic[sizeof NewTopic - 1] = '\0';
		
			//Lookup the channel.
			if ((ChannelStruct = State::LookupChannel(Channel)) == NULL) return; //We aren't in that channel. Why send us a topic?
		
			//Copy in the new topic.
			ChannelStruct->SetTopic(NewTopic);
			
			goto ForwardVerbatim;
		}
		case IRCMSG_TOPICORIGIN:
		{ //Where the topic came from.
			char Channel[256], Setter[256];
			char *Worker = (char*)strchr(Message, '#');
			unsigned WhenSet = 0; //When the topic was set.
			unsigned Inc = 0;
			class ChannelList *ChannelStruct = NULL;
			
			//Get the channel.
			for (; *Worker != ' ' && *Worker != '\0' && Inc < sizeof Channel - 1; ++Inc, ++Worker)
			{
				Channel[Inc] = *Worker;
			}
			Channel[Inc] = '\0';
			
			if (*Worker++ == '\0') return; //Corrupt data.
			
			//Lookup the channel.
			if (!(ChannelStruct = State::LookupChannel(Channel))) return; //Not in that channel.
			
			//Get the setter of the topic.
			for (Inc = 0; *Worker != ' ' && *Worker != '\0' && Inc < sizeof Setter - 1; ++Inc, ++Worker)
			{
				Setter[Inc] = *Worker;
			}
			Setter[Inc] = '\0';
			
			if (*Worker++ == '\0') return; //Another corruption check.
			
			//Get when we set this topic.
			WhenSet = (unsigned)atoll(Worker);
			
			//Copy in the setter.
			ChannelStruct->SetWhoSetTopic(Setter);
			
			
			//Copy in the time it was set.
			ChannelStruct->SetWhenSetTopic(WhenSet);
			
			//Now send it to everyone.
			goto ForwardVerbatim;
			break;
		}
		case IRCMSG_NAMES: //The server is replying to a names command.
		{
			unsigned Inc = 0;
			const char *Search = strchr(Message, '#');
			char NamesChannel[512];
			char NamesNick[64]; //Used by the do-while loop.
			class ChannelList *ChannelStruct = NULL;
			
			//Copy in the channel for this.
			for (; Search[Inc] != ' ' && Search[Inc] != '\0' && Inc < sizeof NamesChannel - 1; ++Inc)
			{
				NamesChannel[Inc] = Search[Inc];
			}
			NamesChannel[Inc] = '\0';
			
			//Can't find the channel.
			if (!(ChannelStruct = State::LookupChannel(NamesChannel))) return;
			
			if (Search[Inc] == '\0') return; //Bad data. Failed.
			
			//Skip past all whitespace and the colon.
			Search += Inc;
			while (*Search == ' ' || *Search == ':') ++Search;
			
			//Process each nick in the names list.
			do
			{
				unsigned char Modes = 0; //Are they OP or something?
				
				while (*Search == ' ') ++Search; //Skip past starting whitespace if reentering loop.
				
				//They are an OP or something. Store their symbol.
				while (*Search == '!' || *Search == '@' || *Search == '#'
					|| *Search == '$' || *Search == '%' || *Search == '&'
					|| *Search == '*' || *Search == '+'
					|| *Search == '=' || *Search == '-' || *Search == '('
					|| *Search == ')' || *Search == '?' || *Search == '~')
				{
					Modes |= State::UserModes_Get_Symbol2Mode(*Search++);
				}
				
				//Get the nickname for this name.
				for (Inc = 0; Search[Inc] != ' ' && Search[Inc] != '\0' && Inc < sizeof NamesNick - 1; ++Inc)
				{
					NamesNick[Inc] = Search[Inc];
				}
				NamesNick[Inc] = '\0';
				
				
				ChannelStruct->AddUser(NamesNick, Modes);
				
			} while ((Search = strchr(Search, ' ')));
			
			//Now send the message to the client.
			snprintf(OutBuf, sizeof OutBuf, "%s\r\n", Message);
			Server::ForwardToAll(OutBuf);
			break;
		}
		case IRCMSG_WHO:
		{			
			const char *Worker = Message;
			
			if ((Worker = strstr(Worker, "352")) == NULL) break;
			
			//Twice, once to get past 352, once to get to the channel name.
			if (!(Worker = SubStrings.Line.WhitespaceJump(Worker))) break;
			if (!(Worker = SubStrings.Line.WhitespaceJump(Worker))) break;
			
			char Channel[128], Nick[128], Ident[128], Mask[128];
		
			//Channel
			SubStrings.CopyUntilC(Channel, sizeof Channel, &Worker, " ", false);
			SubStrings.ASCII.LowerS(Channel);
			
			//Ident
			SubStrings.CopyUntilC(Ident, sizeof Ident, &Worker, " ", false);
			
			//Mask
			SubStrings.CopyUntilC(Mask, sizeof Mask, &Worker, " ", false);
			
			//Skip past server name; we don't care.
			Worker = SubStrings.Line.WhitespaceJump(Worker);
			
			//Nick
			SubStrings.CopyUntilC(Nick, sizeof Nick, &Worker, " ", false);
			
			
			//Now see if they are ignored and we need to ghost them.
			if (Ignore::Check_Separate(Nick, Ident, Mask, NEXUS_IGNORE_VISIBLE))
			{
				//Delete them from our users list.
				class ChannelList *const Chan = State::LookupChannel(Channel);
				if (!Chan) break; ///Baaaad data.
				
				Chan->DelUser(Nick);
				
				//And tell users they are gone.
				snprintf(OutBuf, sizeof OutBuf, ":%s!%s@%s PART %s :\"Ghosted by a NEXUS BNC ignore rule\"\r\n", Nick, Ident, Mask, Channel);
				Server::ForwardToAll(OutBuf);
				break;
			}
			
			goto ForwardVerbatim;
		}
		case IRCMSG_PART:
		case IRCMSG_JOIN:
		case IRCMSG_NICK:
		{ //We need to make sure the ident and IP match the client's
			char NickTest[64];
			unsigned Inc = 0;
			const char *Test = Message + 1; //Skip past the ':' at the start.
			
			for (; Test[Inc] != '!' && Test[Inc] != ' ' && Test[Inc] != '\0' && Inc < sizeof NickTest - 1; ++Inc)
			{ //Copy in the nick for this join/part message.
				NickTest[Inc] = Test[Inc];
			}
			NickTest[Inc] = '\0';
			
			if (!strcmp(NickTest, IRCConfig.Nick)) //This is our nick.
			{ //Since it's our nick, we gotta alter it so that the IP and ident match the client's.
				std::list<struct ClientListStruct>::iterator Iter = ClientListCore.begin();
				
				for (; Iter != ClientListCore.end(); ++Iter)
				{ //Rebuild so it matches their ident and IP.
					if (IRC::AlterMessageOrigin(Message, OutBuf, sizeof OutBuf, &*Iter))
					{ //If we fail to alter the origin just don't send it.
						strncat(OutBuf, "\r\n", sizeof OutBuf - strlen(OutBuf) - 2);
						
						Iter->SendLine(OutBuf);
					}
				}
				
				switch (MsgType)
				{ //Some specific stuff we need to do
					case IRCMSG_NICK:
					{ //if it's our nickname, we need to remember that it's been changed.
						char NewNick[2048];
						
						//update the nick.
						IRC::GetMessageData(Message, NewNick);
						strcpy(IRCConfig.Nick, *NewNick == ':' ? NewNick + 1 : NewNick);
						break;
					}
					case IRCMSG_JOIN:
					{ //We joined a channel.
						char NewChannel[2048];
						class ChannelList *Chan;
						IRC::GetMessageData(Message, NewChannel);
						
						Chan = State::AddChannel(NewChannel);
						Chan->AddUser(IRCConfig.Nick, 0);
						
						//Send a WHO
						snprintf(OutBuf, sizeof OutBuf, "WHO %s\r\n", NewChannel);
						Net::Write(IRCDescriptor, OutBuf, strlen(OutBuf));
						break;
					}
					case IRCMSG_PART:
					{ //We left a channel.
						char GoneChannel[2048], *Search;
						
						IRC::GetMessageData(Message, GoneChannel);
						
						//Delete whatever part message they may have placed. We don't care.
						if ((Search = strchr(GoneChannel, ' '))) *Search = '\0';
						
						//Now delete this parted channel.
						State::DelChannel(GoneChannel);
						break;
					}
					default:
						break;
				}
			}

			//Should we "see" this?
			if (Ignore::Check(Message, NEXUS_IGNORE_VISIBLE)) break;
			
			//Send it to everyone.
			std::string NewM = std::string(Message) + "\r\n";
			
			Server::ForwardToAll(NewM.c_str());
			
			switch (MsgType)
			{
				char Nick[64], Ident[64], Mask[256];
				
				case IRCMSG_NICK:
				{ //Change the nick for them in all channels they exist in.
					std::map<std::string, ChannelList>::iterator Iter = ChannelListCore.begin();
					char NewNick_[64], *NewNick = NewNick_;
					
					//Get their old nick.
					IRC::BreakdownNick(Message, Nick, Ident, Mask);
					IRC::GetMessageData(Message, NewNick_);
					
					if (*NewNick == ':') ++NewNick;
					
					for (; Iter != ChannelListCore.end(); ++Iter)
					{
						ChannelList *Chan = &Iter->second;
						
						Chan->RenameUser(Nick, NewNick);
					}
					break;
				}
				case IRCMSG_PART:
				case IRCMSG_JOIN:
				{ //Add this user to the channel.
					char DaChannel[512], *Search;
					class ChannelList *ChannelStruct;
					
					IRC::BreakdownNick(Message, Nick, Ident, Mask);
					IRC::GetMessageData(Message, DaChannel);
					
					if ((Search = strchr(DaChannel, ' '))) *Search = '\0';
					
					if (!(ChannelStruct = State::LookupChannel(DaChannel))) break;

					if (MsgType == IRCMSG_JOIN)
					{
						ChannelStruct->AddUser(Nick, 0);
					}
					else
					{
						ChannelStruct->DelUser(Nick);
					}
					break;
				}
				default:
					break;
			}
			break;
		}
		case IRCMSG_MODE:
		{
			const char *Worker = strchr(Message, '#');
			char Channel[256], Nick[64];
			struct UserStruct *UserStruct = NULL;
			class ChannelList *ChannelStruct = NULL;
			unsigned Inc = 0;
			char ModeLetter = 0;
			bool SettingMode = false;
			
			if (!Worker) goto ForwardVerbatim; //Not a channel.
			
			
			//Get the channel the mode affects.
			for (; *Worker != ' ' && *Worker != '\0' && Inc < sizeof Channel - 1; ++Inc, ++Worker)
			{
				Channel[Inc] = *Worker;
			}
			Channel[Inc] = '\0';
			
			if (*Worker == '\0') goto ForwardVerbatim; //Gibberish to us but maybe someone can make use of it.
			
			while (*Worker == ' ') ++Worker;
			
			switch (*Worker)
			{ //Determine if we are stopping or starting this mode.
				case '+':
					SettingMode = true;
					break;
				case '-':
					SettingMode = false;
					break;
				default:
					return; //Bad data.
			}
			
			++Worker; //Now on to the letter that tells us what mode.
			
			//Look up the mode by its letter.
			ModeLetter = State::UserModes_Get_Letter2Mode(*Worker);
			
			if (!ModeLetter) goto ForwardVerbatim; //Not something we can deal with.
			
			//Now get the nickname.
			if (!(Worker = strchr(Worker, ' '))) return; //Bad data again.
			
			//Skip past the spaces.
			while (*Worker == ' ') ++Worker;
			
			//Check if this is a mask. If so, return. We are only handling user modes here.
			if (strchr(Worker, '!')) goto ForwardVerbatim;
			
			//Now, actually get the nickname.
			for (Inc = 0; *Worker != '\0' && Inc < sizeof Nick - 1; ++Inc, ++Worker)
			{
				Nick[Inc] = *Worker;
			}
			Nick[Inc] = '\0';
			
			//Look up the channel.
			if (!(ChannelStruct = State::LookupChannel(Channel))) goto ForwardVerbatim; //Can't find the channel.
			
			//Look up this user.
			if (!(UserStruct = ChannelStruct->GetUser(Nick))) goto ForwardVerbatim; //Can't find the nick.
			
			//Now alter their mode accordingly.
			if (SettingMode)
			{
				UserStruct->Modes |= ModeLetter;
			}
			else
			{
				UserStruct->Modes &= ~ModeLetter;
			}
			goto ForwardVerbatim; //Now send the original message to the clients.
		}
		case IRCMSG_KICK:
		{
			char InBuf[2048], *Worker = InBuf;
			char KChan[128], KNick[128];
			unsigned Inc = 0;
			
			/*Get initial kick data into InBuf*/
			IRC::GetMessageData(Message, InBuf);

			
			for (; Inc < sizeof KChan - 1 && *Worker != ' ' && *Worker != '\0'; ++Inc, ++Worker)
			{ /*Channel.*/
				KChan[Inc] = *Worker; /*For all channel comparisons except for display, we want lowercase.*/
			}
			KChan[Inc] = '\0';
			
			if (*Worker == '\0') return; //No nick, just a channel. Malformed.
			
			//Skip to the end of whitespace.
			while (*Worker == ' ') ++Worker;
			
			for (Inc = 0; Inc < sizeof KNick - 1 && Worker[Inc] != ' ' && Worker[Inc] != '\0'; ++Inc)
			{ /*Get the nick.*/
				KNick[Inc] = Worker[Inc];
			}
			KNick[Inc] = '\0';

			if (!strcmp(KNick, IRCConfig.Nick))
			{ /*we have been kicked.*/
				State::DelChannel(KChan);
			}
			else
			{ /*Someone else has been kicked.*/
				class ChannelList *ChanStruct = State::LookupChannel(KChan);
				
				if (!ChanStruct) return; //Can't find the channel.
				
				ChanStruct->DelUser(KNick);
			}
			
			//Now write the message to each client.
			goto ForwardVerbatim;
		}
		case IRCMSG_QUIT:
		{ //Handles quitting per-user.
			char Nick[64], Ident[64], Mask[256];
			IRC::BreakdownNick(Message, Nick, Ident, Mask);
			
			if (!strcmp(Nick, IRCConfig.Nick))
			{ //Uhoh, it's us.
				const char *QuitReason = strchr(Message + 1, ':'); //+ 1 to skip past the FIRST ':'.
				
				if (QuitReason)
				{
					snprintf(OutBuf, sizeof OutBuf,
							":" CONTROL_NICKNAME "!NEXUS@NEXUS PRIVMSG %s :NEXUS was disconnected from the IRC server. "
							"The reason given was \"%s\". NEXUS is shutting down.\r\n", IRCConfig.Nick, QuitReason + 1);
				}
				else
				{
					snprintf(OutBuf, sizeof OutBuf,
							":" CONTROL_NICKNAME "!NEXUS@NEXUS PRIVMSG %s :NEXUS was disconnected from the IRC server. "
							"No reason was provided by the server. NEXUS is shutting down.\r\n", IRCConfig.Nick);
				}

					
				Server::ForwardToAll(OutBuf);
				SubStrings.StripTrailingChars(OutBuf, "\r\n");
				puts(OutBuf);
				
				IRC::Disconnect(); //Close the IRC server connection.
				Server::SendQuit(-1, "IRC server sent NEXUS a QUIT command."); //Tell all clients to quit.
				Net::ShutdownServer(); //Bring down the NEXUS server.
				Exit(1);
			}
			
			//Yay! Not us!
			std::map<std::string, ChannelList>::iterator Iter = ChannelListCore.begin();
			
			for (; Iter != ChannelListCore.end(); ++Iter)
			{
				Iter->second.DelUser(Nick);
			}
			
			//Now forward to all our users.
			snprintf(OutBuf, sizeof OutBuf, "%s\r\n", Message);
			Server::ForwardToAll(OutBuf);
		}
	}
}

static void NEXUS::HandleClientInterface(const char *const Message, struct ClientListStruct *Client)
{ //Processes commands sent to our control nickname.
	const char *Argument = NULL;
	char PrimaryCommand[64];
	unsigned Inc = 0;
	char OutBuf[2048];
	
	for (; Message[Inc] != ' ' && Message[Inc] != '\0' && Inc < sizeof PrimaryCommand - 1; ++Inc)
	{ //Get the main reason we're here.
		PrimaryCommand[Inc] = Message[Inc];
	}
	PrimaryCommand[Inc] = '\0';
	
	//This will be where we find any arguments later.
	for (Argument = Message + Inc; *Argument == ' '; ++Argument);
	
	if (!strcmp(PrimaryCommand, "quit"))
	{
		//Confirm the shutdown.
		snprintf(OutBuf, sizeof OutBuf, ":" CONTROL_NICKNAME "!NEXUS@NEXUS PRIVMSG %s :Shutting down NEXUS.\r\n",
				IRCConfig.Nick);
		Client->SendLine(OutBuf);
		
		
		//Tell all clients to quit.
		Server::SendQuit(-1, "A quit command was received from NEXUS' control nickname.");
		
		//Disconnect from the IRC server.
		IRC::Disconnect();
		
		//Bring down the NEXUS server.
		Net::ShutdownServer();
		
		//Clean up some stuff.
		Scrollback::Shutdown();
		Ignore::Shutdown();
		State::ShutdownChannelList();
		Server::ClientList::Shutdown();
		
		Exit(0);
	}
	else if (!strcmp(PrimaryCommand, "status")) //They want a list of clients and whatnot.
	{
		
		const unsigned ChannelCount = ChannelListCore.size();
		unsigned Inc = 1;
		
		//Count channels.
		
		//Count clients.
		const unsigned ClientCount = ClientListCore.size();
		
		//List all channels we are in.
		snprintf(OutBuf, sizeof OutBuf, ":" CONTROL_NICKNAME "!NEXUS@NEXUS PRIVMSG %s :List of channels NEXUS is in:\r\n",
				IRCConfig.Nick);
		Client->SendLine(OutBuf);
		
		std::map<std::string, ChannelList>::iterator Iter = ChannelListCore.begin();
		
		for (; Iter != ChannelListCore.end(); ++Iter, ++Inc)
		{
			snprintf(OutBuf, sizeof OutBuf, ":" CONTROL_NICKNAME "!NEXUS@NEXUS PRIVMSG %s :[%u/%u] %s\r\n",
					IRCConfig.Nick, Inc, ChannelCount, Iter->second.GetChannelName());
			Client->SendLine(OutBuf);
		}

		//List clients and their info.		
		snprintf(OutBuf, sizeof OutBuf, ":" CONTROL_NICKNAME "!NEXUS@NEXUS PRIVMSG %s :List of clients connected to this NEXUS:\r\n",
				IRCConfig.Nick);
		Client->SendLine(OutBuf);
		
		std::list<struct ClientListStruct>::iterator ClientIter = ClientListCore.begin();
		for (Inc = 1; ClientIter != ClientListCore.end(); ++ClientIter, ++Inc)
		{
			snprintf(OutBuf, sizeof OutBuf, ":" CONTROL_NICKNAME "!NEXUS@NEXUS PRIVMSG %s :[%u/%u]%s Ident: \"%s\" IP: \"%s\"\r\n",
					IRCConfig.Nick, Inc, ClientCount, &*ClientIter == Client ? " (YOU)" : "",
					ClientIter->Ident, ClientIter->IP);
			Client->SendLine(OutBuf);
		}
		
		snprintf(OutBuf, sizeof OutBuf, ":" CONTROL_NICKNAME "!NEXUS@NEXUS PRIVMSG %s :End of status report.\r\n",
				IRCConfig.Nick);
		Client->SendLine(OutBuf);
		return;
	}
	else if (!strcmp(PrimaryCommand, "ignore"))
	{
		const char *Iter = Argument;
		
		char VHost[1024] = { 0 };
		char Blockages[1024] = { 0 };

		//Get the vhost to perform the operation on
		SubStrings.CopyUntilC(VHost, sizeof VHost, &Iter, " ", true);
		
		//Get the list of blockages
		SubStrings.Copy(Blockages, Iter, sizeof Blockages);
	
		const char *I2 = Blockages;
		
		char CurFlag[64];
		
		while (SubStrings.CopyUntilC(CurFlag, sizeof CurFlag, &I2, ",", true))
		{
			unsigned Flag = 0;
			
			bool Adding = false;
			const char *FlagData = CurFlag + 1;
			switch (*CurFlag)
			{ //Adding or removing?
				case '-':
					Adding = false;
					break;
				case '+':
					Adding = true;
					break;
				default:
					snprintf(OutBuf, sizeof OutBuf, ":" CONTROL_NICKNAME "!NEXUS@NEXUS PRIVMSG %s :Missing operator for blockages list.\r\n", IRCConfig.Nick);
					Client->SendLine(OutBuf);
					break;
			}
			
			if (!strcmp(FlagData, "CHANMSG"))
			{
				Flag |= NEXUS_IGNORE_CHANMSG;
			}
			else if (!strcmp(FlagData, "PRIVMSG"))
			{
				Flag |= NEXUS_IGNORE_PRIVMSG;
			}
			else if (!strcmp(FlagData, "NOTICE"))
			{
				Flag |= NEXUS_IGNORE_NOTICE;
			}
			else if (!strcmp(FlagData, "VISIBLE"))
			{
				Flag |= NEXUS_IGNORE_VISIBLE;
			}
			else if (!strcmp(FlagData, "ALL"))
			{
				Flag |= NEXUS_IGNORE_ALL;
			}
			else
			{
				snprintf(OutBuf, sizeof OutBuf, ":" CONTROL_NICKNAME "!NEXUS@NEXUS PRIVMSG %s :Bad ignore flag \"%s\"\r\n", IRCConfig.Nick, FlagData);
				Client->SendLine(OutBuf);
				continue;
			}
			
			
			
			if (Ignore::Modify(VHost, Adding, Flag))
			{
				snprintf(OutBuf, sizeof OutBuf, ":" CONTROL_NICKNAME "!NEXUS@NEXUS PRIVMSG %s :Ignore flag %s %s for %s\r\n", IRCConfig.Nick,
						FlagData, Adding ? "enabled" : "disabled", VHost);
				Client->SendLine(OutBuf);
				continue;
			}
			else
			{
				snprintf(OutBuf, sizeof OutBuf, ":" CONTROL_NICKNAME "!NEXUS@NEXUS PRIVMSG %s :Failed to %s flag %s for %s\r\n", IRCConfig.Nick,
						Adding ? "enable" : "disable", FlagData, VHost);
				Client->SendLine(OutBuf);
				continue;
			}
		}
		
		
	}	
	else
	{
		snprintf(OutBuf, sizeof OutBuf, ":" CONTROL_NICKNAME "!NEXUS@NEXUS PRIVMSG %s :Command unrecognized.\r\n", IRCConfig.Nick);
		Client->SendLine(OutBuf);
		return;
	}
}
