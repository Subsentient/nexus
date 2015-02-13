/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**Controls the recording, formatting, and reporting of all scrollback.**/

#include "scrollback.h"
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

struct ScrollbackList *ScrollbackCore;

struct ScrollbackList *Scrollback_AddMsg(const char *Msg, const char *Origin, const char *Target, time_t Time)
{
	struct ScrollbackList *Worker = ScrollbackCore;
	
	if (!Worker)
	{
		Worker = ScrollbackCore = calloc(1, sizeof(struct ScrollbackList));
	}
	else
	{
		for (; Worker->Next; Worker = Worker->Next); //Skip to a free Next.
		
		Worker->Next = calloc(1, sizeof(struct ScrollbackList));
		Worker->Next->Prev = Worker;
		Worker = Worker->Next;
	}
	
		
	Worker->Msg = malloc(strlen(Msg) + 1);
	strcpy((char*)Worker->Msg, Msg); //These pointers are constant but it's safe to cast them to mutable.
	
	if (Origin)
	{ //if it's null, it's from us and we just want it to show in scrollback.
		Worker->Origin = malloc(strlen(Origin) + 1);
		strcpy((char*)Worker->Origin, Origin);
	}
	
	if (Target)
	{ //if it's null we send to ourselves.
		Worker->Target = malloc(strlen(Target) + 1);
		strcpy((char*)Worker->Target, Target);
	}
	
	Worker->Time = Time;
	
	return Worker;
}

void Scrollback_DelMsg(struct ScrollbackList *ToDel)
{
	
	//First we must delete the objects for this thing.
	free((void*)ToDel->Msg);
	free((void*)ToDel->Origin);
	if (ToDel->Target) free((void*)ToDel->Target);
	
	//Then we can remove it from the list.
	if (ToDel == ScrollbackCore)
	{
		if (ToDel->Next)
		{
			ScrollbackCore = ToDel->Next;
			ScrollbackCore->Prev = NULL;
			free(ToDel);
		}
		else
		{
			ScrollbackCore = NULL;
			free(ToDel);
		}
	}
	else
	{
		if (ToDel->Next) ToDel->Next->Prev = ToDel->Prev;
		ToDel->Prev->Next = ToDel->Next;
		free(ToDel);
	}
}

void Scrollback_Shutdown(void)
{
	struct ScrollbackList *Worker = ScrollbackCore, *Next;
	
	for (; Worker; Worker = Next)
	{
		//Free subobjects.
		free((void*)Worker->Msg);
		free((void*)Worker->Origin);
		if (Worker->Target) free((void*)Worker->Target);
		
		//Then we can nuke the rest.
		Next = Worker->Next;
		free(Worker);
	}
}

void Scrollback_Reap(void)
{

		//Reap expired scrollback.
		const time_t TimeCompare = time(NULL);
		
	SBLoop:
		for (struct ScrollbackList *SBWorker = ScrollbackCore; SBWorker; SBWorker = SBWorker->Next)
		{
			if (SBWorker->Time + NEXUSConfig.ScrollbackKeepTime < TimeCompare)
			{ //Expired. Reap it.
				Scrollback_DelMsg(SBWorker);
				goto SBLoop;
			}
		}
}

