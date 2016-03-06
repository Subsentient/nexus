/*This file is part of libcpsl.
 * libcpsl is public domain software; you may use it however you please.
 * Refer to the file UNLICENSE.txt for more information.
 * 2016, by Subsentient, the white rat hiding under your chair. Don't look.
 */

#ifndef __CPSL_INTERNALHEADER__
#define __CPSL_INTERNALHEADER__

enum { false, true };

#include <stddef.h> /*For size_t*/
#include <stdint.h>

//NULL is defined in a bunch of places on gcc/clang/glibc compilers/platforms, so let's put it after the headers as to not be caught by surprise.
#ifndef NULL
#define NULL ((void*)0)
#endif //NULL

struct CPSL_Allocator
{
	void *(*malloc)(const size_t Size);
	void (*free)(void *const Data);
	void *(*realloc)(void *const Ptr, const size_t NewSize);
};

struct CPSL_DynArray
{
	unsigned PerElementSize;
	unsigned AllocatedElements;
};


//Static functions.
static inline void MemCopy(void *const Dest, const void *const Src, const size_t ByteCount)
{
	register const uint8_t *const Stopper = (uint8_t*)Src + ByteCount;
	register const uint8_t *SrcWorker = Src;
	register uint8_t *DestWorker = Dest;
	
	for (; SrcWorker != Stopper; ++SrcWorker, ++DestWorker)
	{
		*DestWorker = *SrcWorker;
	}
}

static inline void MemWipe(void *const Dest, const size_t ByteCount)
{
	register uint8_t *W = Dest;
	register uint8_t *const Stopper = W + ByteCount;
	
	for (; W != Stopper; ++W) *W = 0;
}

static inline size_t StringLen(register const char *String)
{
	register size_t Inc = 0;
	
	while (*String) ++Inc, ++String;
	
	return Inc;
}

static inline uint8_t StringCmp(register const char *String1,register const char *String2)
{ //It's uint8_t so I don't have to order headers so that libcpsl.h would need to be first.
	for (; *String1 && *String2; ++String1, ++String2)
	{
		if (*String1 != *String2) return false;
	}
	
	if (*String1 != *String2) return false; /*If one string is shorter than the other*/
	
	return true;
}

extern struct CPSL_Allocator Alloc;

#endif //__CPSL_INTERNALHEADER__
