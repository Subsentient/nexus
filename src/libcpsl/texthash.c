/*This file is part of libcpsl.
 * libcpsl is public domain software; you may use it however you please.
 * Refer to the file UNLICENSE.txt for more information.
 * 2016, by Subsentient, the white rat hiding under your chair. Don't look.
 */

#include <limits.h>
#include "cpslinternal.h"
#include "libcpsl.h"

//Types and macros


//Static function prototypes
static void CPSL_HashStatic_DestroySublist(struct CPSL_HashElement **const List);
static CPSL_Bool CPSL_HashStatic_NewListNode(struct CPSL_HashElement **ListPP, const char *Key, const void *InData, const unsigned DataSize);
static struct CPSL_HashElement **CPSL_HashStatic_JumpToList(const char FirstChar, struct CPSL_HashElement **List);
static CPSL_Bool CPSL_HashStatic_DeleteListNode(struct CPSL_HashElement **const ListPP, struct CPSL_HashElement *const ToDelete);
static struct CPSL_HashElement *CPSL_HashStatic_LookupListNode(struct CPSL_HashElement *List, const char *const Key);

//Function definitions
CPSL_Hash CPSL_Hash_New(void)
{
	CPSL_Hash NewHandle = Alloc.malloc(sizeof(struct CPSL_HashElement*) * HASH_NUMLISTS);

	for (register int Inc = 0; Inc < HASH_NUMLISTS; ++Inc)
	{ /*Not as efficient, but I hear some architectures' internal representation of a null pointer is not all bits set to zero.
		* Usually I wouldn't give two shits and a popsicle, but I want this library to be very portable.*/
		NewHandle[Inc] = NULL;
	}
	
	return NewHandle;
}

static struct CPSL_HashElement *CPSL_HashStatic_LookupListNode(struct CPSL_HashElement *List, const char *const Key)
{
	for (; List; List = List->Next)
	{
		if (StringCmp(List->Key, Key)) return List;
	}
	return NULL;
}


void CPSL_Hash_Destroy(CPSL_Hash Hash)
{
	if (!Hash) return;
	
	register CPSL_Hash Stopper = Hash + HASH_NUMLISTS;
	register CPSL_Hash Worker = Hash;

	for (; Worker != Stopper; ++Worker)
	{ //Destroy all populated lists.
		CPSL_HashStatic_DestroySublist(Worker);
	}
	
	Alloc.free(Hash); //And we release the array of pointers now.
}

static void CPSL_HashStatic_DestroySublist(struct CPSL_HashElement **const List)
{
	if (!List) return;
	
	register struct CPSL_HashElement *Next = NULL, *Worker = *List;
	
	for (; Worker; Worker = Next)
	{
		Next = Worker->Next;
		Alloc.free(Worker);
	}
	
	*List = NULL;
}

CPSL_Bool CPSL_Hash_Set(const CPSL_Hash Hash, const char *const Key, const void *const InData, const size_t InDataSize)
{
	if (!Key || !Hash) return false;
	
	struct CPSL_HashElement **List = CPSL_HashStatic_JumpToList(*Key, Hash);
	struct CPSL_HashElement *Element = CPSL_HashStatic_LookupListNode(*List, Key);
	
	if (!Element)
	{
		if (!InData || !InDataSize) //They wanted to delete something. We don't have to.
		{
			return true;
		}
		
		//They want to add something new, and we need to create a new list.
		return CPSL_HashStatic_NewListNode(List, Key, InData, sizeof InData);
	}
	
	if (!InData || !InDataSize) //They want to delete something, and looks like we got something to delete.
	{
		return CPSL_HashStatic_DeleteListNode(List, Element);
	}
	
	//They want to edit an existing value.
	
	//Release old data.
	Alloc.free(Element->Data);
	
	//Allocate space for new.
	Element->Data = Alloc.malloc(InDataSize);
	
	//Load in the new data.
	MemCopy(Element->Data, InData, InDataSize);
	
	//Remember the data's size.
	Element->DataSize = InDataSize;
	
	return true;
}

struct CPSL_HashElement *CPSL_Hash_Get(const CPSL_Hash Hash, const char *const Key, void *OutData)
{
	struct CPSL_HashElement **List = CPSL_HashStatic_JumpToList(*Key, Hash);
	struct CPSL_HashElement *Element = CPSL_HashStatic_LookupListNode(*List, Key);
	
	if (!Element) return NULL;
	
	if (OutData != NULL) MemCopy(OutData, Element->Data, Element->DataSize);
	
	return Element;
}

static struct CPSL_HashElement **CPSL_HashStatic_JumpToList(const char FirstChar, struct CPSL_HashElement **List)
{
	if (!FirstChar) return NULL; //Empty string.
	
	if ( (*(uint8_t*)&FirstChar) > SCHAR_MAX) return List; //Not ASCII, we have a special list for that.
	else return List + FirstChar; //Regular ASCII.
}


static CPSL_Bool CPSL_HashStatic_NewListNode(struct CPSL_HashElement **ListPP, const char *Key, const void *InData, const unsigned DataSize)
{
	if (!ListPP) return false;
	
	struct CPSL_HashElement *OldHead = *ListPP;

	//Replace head with a fresh one.
	*ListPP = Alloc.malloc(sizeof(struct CPSL_HashElement));

	//Previous head was non-NULL, so we let that previous head be our ->Next
	if (OldHead)
	{
		(*ListPP)->Next = OldHead;
	}
	else
	{
		(*ListPP)->Next = NULL;
	}
	
	const size_t KeyLen = StringLen(Key) + 1;
	
	//Allocate space for the data and remember its size.
	(*ListPP)->Data = Alloc.malloc(DataSize);
	(*ListPP)->Key = Alloc.malloc(KeyLen);
	(*ListPP)->DataSize = DataSize;
	
	//Now we copy in the data and key.
	MemCopy((void*)(*ListPP)->Key, Key, KeyLen);
	MemCopy((*ListPP)->Data, InData, DataSize);
	
	return true;
}

static CPSL_Bool CPSL_HashStatic_DeleteListNode(struct CPSL_HashElement **const ListPP, struct CPSL_HashElement *const ToDelete)
{ //You know, I kinda enjoy trolling people with weird pointer declarations, like ListPP here.
	if (!ListPP || !ToDelete || !*ListPP) return false;
	
	if (*ListPP == ToDelete)
	{ //Fast if we got just one.
		*ListPP = ToDelete->Next;
		Alloc.free(ToDelete);
		return true;
	}
	
	struct CPSL_HashElement *Worker = *ListPP;
	
	for (; Worker->Next; Worker = Worker->Next)
	{
		if (Worker->Next == ToDelete)
		{
			Worker->Next = Worker->Next->Next; //Might be NULL but doesn't matter.
			if (ToDelete->Data) Alloc.free(ToDelete->Data);
			if (ToDelete->Key) Alloc.free((void*)ToDelete->Key);
			Alloc.free(ToDelete);
			return true;
		}
	}
	
	return false;
}
	
