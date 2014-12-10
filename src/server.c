/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "server.h"
#include "netcore.h"
/**Specifies how we talk to clients and handles stuff like that.**/

//Globals
struct ClientTree *ClientTreeCore;

//Functions
struct ClientTree *Server_ClientTree_Add(const struct ClientTree *const InStruct)
{
	struct ClientTree *Worker = ClientTreeCore, *TempNext, *TempPrev;
	
	if (!ClientTreeCore)
	{
		Worker = ClientTreeCore = calloc(1, sizeof(struct ClientTree)); //use calloc to zero it out
	}
	else
	{
		while (Worker->Next) Worker = Worker->Next;
		Worker->Next = calloc(1, sizeof(struct ClientTree));
		Worker->Next->Prev = Worker;
		Worker = Worker->Next;
	}
	
	TempNext = Worker->Next;
	TempPrev = Worker->Prev;
	
	*Worker = *InStruct;
	
	Worker->Next = TempNext;
	Worker->Prev = TempPrev;
	
	return Worker;
}


void Server_ClientTree_Shutdown(void)
{
	struct ClientTree *Worker = ClientTreeCore, *Next;

	for (; Worker; Worker = Next)
	{
		Next = Worker->Next;
		free(Worker);
	}
	
	ClientTreeCore = NULL;
}

bool Server_ClientTree_Del(const int Descriptor)
{
	struct ClientTree *Worker = ClientTreeCore;
	
	if (!ClientTreeCore) return false;
	
	for (; Worker; Worker = Worker->Next)
	{
		if (Worker->Descriptor == Descriptor)
		{ //Match.
			if (Worker == ClientTreeCore)
			{
				if (Worker->Next)
				{ //We're the first one but there are others ahead of us.
					ClientTreeCore = Worker->Next;
					ClientTreeCore->Prev = NULL;
					free(Worker);
				}
				else
				{ //Just us.
					free(Worker);
					ClientTreeCore = NULL;
				}
			}
			else
			{
				Worker->Prev->Next = Worker->Next;
				if (Worker->Next) Worker->Next->Prev = Worker->Prev;
				free(Worker);
			}
			
			return true;
		}
	}
	
	return false;
}

bool Server_ForwardToAll(const char *const InStream)
{ //This function sends the provided text stream to all clients. Very simple.
	struct ClientTree *Worker = ClientTreeCore;
	
	if (!Worker) return false;
	
	for (; Worker; Worker = Worker->Next)
	{
		Net_Write(Worker->Descriptor, (void*)InStream, strlen(InStream));
	}
	return true;
}
