/*This file is part of libcpsl.
 * libcpsl is public domain software; you may use it however you please.
 * Refer to the file UNLICENSE.txt for more information.
 * 2016, by Subsentient, the white rat hiding under your chair. Don't look.
 */


///You know, I don't think the dynarray thing is as useful as it could be.
#include "libcpsl.h"
#include "cpslinternal.h"

static void *CPSL_DynArray_ChangeSize(void *Array, const int NumElementsToChange);

void *CPSL_DynArray_New(const unsigned PerElementSize, const unsigned NumElementsToAllocate)
{
	return CPSL_DynArray_New_Inz(PerElementSize, NumElementsToAllocate, NULL, 0);
}

void *CPSL_DynArray_New_Inz(const unsigned PerElementSize, const unsigned NumElementsToAllocate, const void *Initializer, const unsigned NumInitializerElements)
{
	//No size requested.
	if (PerElementSize == 0) return NULL;
	
	//No allocated count.
	if (NumElementsToAllocate < 1) return NULL;
	
	//Initializer is bigger than capacity.
	if (Initializer && NumInitializerElements > NumElementsToAllocate) return NULL;
	
	struct CPSL_DynArray *NewArray = Alloc.malloc(sizeof(struct CPSL_DynArray) + (PerElementSize * NumElementsToAllocate));
	
	NewArray->PerElementSize = PerElementSize; //The size of each array element.
	NewArray->AllocatedElements = NumElementsToAllocate; //The number of cells for these elements we've allocated.
	
	
	//Set up the initializer.
	if (Initializer && NumInitializerElements > 0)
	{
		unsigned Inc = 0;
		
		uint8_t *Worker = (uint8_t*)(NewArray + 1);
		for (; Inc < NumInitializerElements; ++Inc)
		{
			MemCopy(Worker + (PerElementSize * Inc), (uint8_t*)Initializer + (PerElementSize * Inc), PerElementSize);
		}
	}
	
	return NewArray + 1; //We return the chunk that has the user's data.
}

void CPSL_DynArray_Destroy(void *Array)
{
	void *Core = (struct CPSL_DynArray*)Array - 1;
	Alloc.free(Core);
}

unsigned CPSL_DynArray_Capacity(void *Array)
{
	return ((struct CPSL_DynArray*)Array - 1)->AllocatedElements;
}

unsigned CPSL_DynArray_ElementSize(void *Array)
{
	return ((struct CPSL_DynArray*)Array - 1)->PerElementSize;
}

void *CPSL_DynArray_Grow(void *Array, const unsigned NumElementsToAdd)
{
	return CPSL_DynArray_ChangeSize(Array, NumElementsToAdd);
}

void *CPSL_DynArray_Shrink(void *Array, const unsigned NumElementsToRemove)
{
	return CPSL_DynArray_ChangeSize(Array, -(int)NumElementsToRemove); //Exploiting signedness.
}

static void *CPSL_DynArray_ChangeSize(void *Array, const int NumElementsToChange)
{
	struct CPSL_DynArray *Core = (struct CPSL_DynArray*)Array - 1;
	const unsigned CurrentSize = Core->AllocatedElements;
	const unsigned PerElementSize = Core->PerElementSize;
	
	if (Alloc.realloc) //We have a realloc, which makes things much easier.
	{
		struct CPSL_DynArray *New = Alloc.realloc(Core, sizeof(struct CPSL_DynArray) + (PerElementSize * (CurrentSize + NumElementsToChange)));
		New->AllocatedElements += NumElementsToChange;
		return New + 1;
	}
	
	//We don't have realloc, so we gotta do it manually.
	struct CPSL_DynArray *New = Alloc.malloc(sizeof(struct CPSL_DynArray) + (PerElementSize * (CurrentSize + NumElementsToChange)));
	MemCopy(New, Core, sizeof(struct CPSL_DynArray) + (PerElementSize * CurrentSize));
	Alloc.free(Core); //We'll definitely have a different pointer without realloc, so we gotta delete the old one.
	New->AllocatedElements += NumElementsToChange;
	
	return New + 1;
}

