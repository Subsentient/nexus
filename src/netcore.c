/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**This file is responsible for networking for clients that connect to **us**.**/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include "netcore.h"
#include "config.h"
#include "server.h"

/*Globals*/
int ServerDescriptor, IRCDescriptor;
static int SocketFamily;

/*Functions*/

struct NetReadReturn Net_Read(int Descriptor, void *OutStream_, unsigned MaxLength, bool IsText)
{
	int Status = 0;
	unsigned char Byte = 0;
	unsigned char *OutStream = OutStream_;
	unsigned Inc = 0;
	struct NetReadReturn ReturnVal;
	
	do
	{
		Status = recv(Descriptor, &Byte, 1, MSG_DONTWAIT);

		*OutStream++ = Byte;

		if (IsText && (Byte == '\n' || Byte == '\0'))
		{
			break;
		}
		
		if (Status == -1 && errno == EWOULDBLOCK)
		{
			goto ReturnTime;
		}

	} while (++Inc, Status > 0 && Inc < MaxLength);
	
	if (IsText)
	{ //Delete the \r\n at the end since it really does us no good.
		*OutStream = '\0';
		if (*(OutStream - 1) == '\r') *(OutStream - 1) = '\0';
	}

ReturnTime:
	ReturnVal.Status = Status;
	ReturnVal.Errno = errno;
	return ReturnVal;
}

bool Net_Write(int const Descriptor, void *InMsg, unsigned WriteSize)
{
	unsigned Transferred = 0, TotalTransferred = 0;

	do
	{
		Transferred = send(Descriptor, InMsg, (WriteSize - TotalTransferred), 0);

		if (Transferred == -1) //Don't say a word. I mean it.
		{
			return false;
		}

		TotalTransferred += Transferred;
	} while (WriteSize > TotalTransferred);

	return true;
}

bool Net_Connect(const char *InHost, unsigned short PortNum, int *SocketDescriptor_)
{

	char *FailMsg = "Failed to establish a connection to the server:";
	struct sockaddr_in SocketStruct;
	struct hostent *HostnameStruct;
	
	memset(&SocketStruct, 0, sizeof(SocketStruct));
	
	if ((HostnameStruct = gethostbyname(InHost)) == NULL)
	{
		
		fprintf(stderr, "Failed to resolve hostname \"%s\".", InHost);
		
		return 0;
	}
	
	if ((*SocketDescriptor_ = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror(FailMsg);
		return false;
	}
	
	memcpy(&SocketStruct.sin_addr, HostnameStruct->h_addr_list[0], HostnameStruct->h_length);
	
	SocketStruct.sin_family = AF_INET;
	SocketStruct.sin_port = htons(PortNum);
	
	if (connect(*SocketDescriptor_, (void*)&SocketStruct, sizeof SocketStruct) != 0)
	{
		
		fprintf(stderr, "Failed to connect to server \"%s\".\n", InHost);
		*SocketDescriptor_ = 0;
		return false;
	}

	return true;
}

bool Net_InitServer(unsigned short PortNum)
{
	struct addrinfo BStruct, *Res = NULL;
	char AsciiPort[16] =  { '\0' };
	int True = true;
	int GAIExit;

	snprintf(AsciiPort, sizeof AsciiPort, "%hu", PortNum);

	memset(&BStruct, 0, sizeof(struct addrinfo));

	BStruct.ai_family = AF_UNSPEC;
	BStruct.ai_socktype = SOCK_STREAM;
	BStruct.ai_flags = AI_PASSIVE;

	if ((GAIExit = getaddrinfo(NULL, AsciiPort, &BStruct, &Res)) != 0)
	{
		fprintf(stderr, "Failed to getaddrinfo(): %s\n", gai_strerror(GAIExit));
		return false;
	}

	SocketFamily = Res->ai_family;

	if (!(ServerDescriptor = socket(Res->ai_family, Res->ai_socktype, Res->ai_protocol)))
	{
		fprintf(stderr, "Failed to open a socket on port %hu.\n", PortNum);
		return false;
	}

	setsockopt(ServerDescriptor, SOL_SOCKET, SO_REUSEADDR, &True, sizeof(int));

	if (bind(ServerDescriptor, Res->ai_addr, Res->ai_addrlen) == -1)
	{
		perror("bind()");
		return false;
	}

	listen(ServerDescriptor, NEXUSConfig.MaxSimulConnections);

	freeaddrinfo(Res);

	fcntl(ServerDescriptor, F_SETFL, O_NONBLOCK); //Set nonblocking. Necessary for our single-threaded model.

	return true;
}

void Net_ShutdownServer(void)
{
	struct ClientTree *Worker = ClientTreeCore;
	
	//Close all connections to clients.
	for (; Worker; Worker = Worker->Next)
	{
		close(Worker->Descriptor);
	}
	
	Server_ClientTree_Shutdown(); //Free list of clients.
	close(ServerDescriptor); //Close the main server socket.
	ServerDescriptor = 0;
}

//We put this in our main loop to scan for clients who want to connect.
bool Net_AcceptClient(int *const OutDescriptor, char *const OutIPAddr, unsigned IPAddrMaxLen)
{
	struct sockaddr ClientInfo;
	struct sockaddr_in Addr;
	socklen_t SockaddrSize = sizeof(struct sockaddr);
	socklen_t AddrSize = sizeof Addr;
	int ClientDescriptor = 0;
	
	
	memset(&ClientInfo, 0, sizeof ClientInfo); //I don't know the spec for this structure, so suck my memset.
	

	if ((ClientDescriptor = accept(ServerDescriptor, &ClientInfo, &SockaddrSize)) == -1)
	{
		if (errno == EWOULDBLOCK)
		{ //No client wants us right now. Not an error.
			usleep(50000); //Sleep 0.05 seconds.
			return false;
		}
		else
		{ //Something more serious than no client.
			fprintf(stderr, "Failed to accept()!\n");
			Net_ShutdownServer();
			exit(1);
		}
	}
	
	
	//Get client IP.
	memset(&Addr, 0, AddrSize);
	getpeername(ClientDescriptor, (void*)&Addr, &AddrSize);
	inet_ntop(SocketFamily, (void*)&Addr.sin_addr, OutIPAddr, IPAddrMaxLen); //Copy it into OutIPAddr.

	*OutDescriptor = ClientDescriptor; //Give them their descriptor.

	return true;
}
