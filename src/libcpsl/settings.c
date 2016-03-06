/*This file is part of libcpsl.
 * libcpsl is public domain software; you may use it however you please.
 * Refer to the file UNLICENSE.txt for more information.
 * 2016, by Subsentient, the white rat hiding under your chair. Don't look.
 */
 
#include "libcpsl.h"
#include "cpslinternal.h"

struct CPSL_Allocator Alloc;

void CPSL_Configure(void *(*malloc)(size_t), void (*free)(void *const), void *(*realloc)(void *const, const size_t))
{
	Alloc.malloc = malloc;
	Alloc.free = free;
	//Realloc is optional, actually. Just more efficient to have a working implementation of realloc.
	Alloc.realloc = realloc;
}
