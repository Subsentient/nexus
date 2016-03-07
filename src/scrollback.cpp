/*NEXUS IRC session BNC, by Subsentient. This software is public domain.*/

/**Controls the recording, formatting, and reporting of all scrollback.**/

#include "scrollback.h"
#include "config.h"
#include "nexus.h"

#include "libcpsl/libcpsl.h"
#include "substrings/substrings.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

struct ScrollbackList *ScrollbackCore;

struct ScrollbackList *Scrollback_AddMsg(const char *Msg, const char *Origin, const char *Target, time_t Time)
{
	struct ScrollbackList *Worker = ScrollbackCore;
	
	if (!Worker)
	{
		Worker = ScrollbackCore = (struct ScrollbackList*)CPSL_List_NewList(sizeof(struct ScrollbackList));
	}
	else Worker = (struct ScrollbackList*)CPSL_List_AddNode((struct CPSL_List*)ScrollbackCore);
	
	memset((struct CPSL_List*)Worker + 1, 0, sizeof(struct ScrollbackList) - sizeof(struct CPSL_List));
		
	Worker->Msg = (const char*)malloc(strlen(Msg) + 1);
	strcpy((char*)Worker->Msg, Msg); //These pointers are constant but it's safe to cast them to mutable.
	
	if (Origin)
	{ //if it's null, it's from us and we just want it to show in scrollback.
		Worker->Origin = (const char*)malloc(strlen(Origin) + 1);
		strcpy((char*)Worker->Origin, Origin);
	}
	
	if (Target)
	{ //if it's null we send to ourselves.
		Worker->Target = (const char*)malloc(strlen(Target) + 1);
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
	
	ScrollbackCore = (struct ScrollbackList*)CPSL_List_DeleteNode((struct CPSL_List*)ToDel);
}

void Scrollback_Shutdown(void)
{
	struct ScrollbackList *Worker = ScrollbackCore;
	
	for (; Worker; Worker = CPSL_LNEXT(Worker))
	{
		//Free subobjects.
		free((void*)Worker->Msg);
		free((void*)Worker->Origin);
		if (Worker->Target) free((void*)Worker->Target);
		
	}
	
	CPSL_List_DestroyList((struct CPSL_List*)ScrollbackCore);
	
	ScrollbackCore = NULL;
}

void Scrollback_Reap(void)
{
	//Reap expired scrollback.
	const time_t TimeCompare = time(NULL);
	
SBLoop:
	for (struct ScrollbackList *SBWorker = ScrollbackCore; SBWorker; SBWorker = CPSL_LNEXT(SBWorker))
	{
		if (SBWorker->Time + NEXUSConfig.ScrollbackKeepTime < TimeCompare)
		{ //Expired. Reap it.
			Scrollback_DelMsg(SBWorker);
			goto SBLoop;
		}
	}
}

