/*This file is part of libcpsl.
 * libcpsl is public domain software; you may use it however you please.
 * Refer to the file UNLICENSE.txt for more information.
 * 2016, by Subsentient, the white rat hiding under your chair. Don't look.
 */

/** Note to any authors: Do NOT add C99/C11 constructs in this header. We must be able to use this from C89.**/

#ifndef __LIBCPSL_H__
#define __LIBCPSL_H__

#define CPSL_MAJORVERSION 0x00
#define CPSL_MINORVERSION 0x01

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


#include <stddef.h> /*For size_t*/

/**Types**/
typedef signed char CPSL_Bool;

/*Our list implementation can use 48 bytes on a 64-bit platform, but it's designed to be fast and versatile more than it is memory efficient.*/
struct CPSL_List
{
	void *Data; /*USERS: You'll need to use this if you are in C++ and want a list of non-POD types. Don't do the type punning for non-POD types.
	* The C++ standard says that's undefined behaviour and might not do what you want at all. The type punning could get you reading from the vtable.
	* If you DO want to do the type punning and it's safe, then this member can be ignored. It's just one pointer.
	* USERS: You are responsible for deleting anything you allocate and attach to Data. If you don't delete it yourself, memory leak.
	*/
	
	/**ATTENTION USERS: Don't change anything below here directly, use the list functions for that! You can read them though.**/
	struct CPSL_List *Next; /*Points to the next element in the list. Will be NULL if you are at the end.*/
	struct CPSL_List *Prev; /*Points to the previous element in the list. Will be NULL if used on Head.*/
	struct CPSL_List **Head; /*Points to a pointer that points to the list head.*/
	struct CPSL_List **End; /*Points to a pointer that points to the list end.*/
	const unsigned *PerElementSize; /*Points to an unsigned int that holds the size per element. There's only one for each entire list.*/
};

struct CPSL_HashElement
{
	struct CPSL_HashElement *Next;
	const char *Key;
	void *Data;
	unsigned DataSize;
};

#define HASH_NUMLISTS 127

typedef struct CPSL_HashElement **CPSL_Hash;

/**Functions**/


void CPSL_Hash_Destroy(CPSL_Hash Hash);
struct CPSL_HashElement *CPSL_Hash_Get(const CPSL_Hash Hash, const char *const Key, void *OutData);
CPSL_Hash CPSL_Hash_New(void);
CPSL_Bool CPSL_Hash_Set(const CPSL_Hash Hash, const char *const Key, const void *const InData, const size_t InDataSize);

/*The initialization function. You don't need to provide a realloc() implementation, but it's more efficient if you do.*/
void CPSL_Configure(void *(*malloc)(size_t), void (*free)(void *const), void *(*realloc_isoptional)(void *const, const size_t));

/*Dynamic array functions.*/
void *CPSL_DynArray_New_Inz(const unsigned PerElementSize, const unsigned NumElementsToAllocate, const void *Initializer, const unsigned NumInitializerElements);
void *CPSL_DynArray_New(const unsigned PerElementSize, const unsigned NumElementsToAllocate);
void CPSL_DynArray_Destroy(void *Array);
unsigned CPSL_DynArray_Capacity(void *Array);
unsigned CPSL_DynArray_ElementSize(void *Array);
void *CPSL_DynArray_Grow(void *Array, const unsigned NumElementsToAdd);
void *CPSL_DynArray_Shrink(void *Array, const unsigned NumElementsToRemove);

/*Linked list functions*/

/**With CPSL_List_NewList(), PerElementSize should be sizeof(struct CPSL_List) unless you want to do the type punning trick,
 * (which is why we ask you), but make sure it's at least sizeof(struct CPSL_List) no matter what.
 * It'll return NULL if PerElementSize is too small.
 **/
struct CPSL_List *CPSL_List_NewList(const unsigned PerElementSize);

/** Really, with both of these below functions, it looks up *->Head from any element you give it,
 * so you can just pass it any arbitrary member of a list and it'll figure Head out for itself.
 **/
CPSL_Bool CPSL_List_DestroyList(struct CPSL_List *AnyListElement);

struct CPSL_List *CPSL_List_AddNode(struct CPSL_List *AnyListElement);

/**CPSL_List_DeleteNode() can change *->Head, so it's best not to rely on the value of *->Head, but instead ->Head.
 * It will delete the entire list if you are deleting the last element. You're warned.
 **/
struct CPSL_List *CPSL_List_DeleteNode(struct CPSL_List *NodeToDelete);


/**Globals**/



/**Macros and such.**/

#ifdef __cplusplus

} /*extern "C"*/

/*We need this template for list next-ing because we can't do implicit conversion FROM void* in C++.*/
template <typename T>
static inline T CPSL_LHEAD(T List)
{
	return reinterpret_cast<T>( *(((struct CPSL_List*)List)->Head) );
}

template <typename T>
static inline T CPSL_LEND(T List)
{
	return reinterpret_cast<T>( *(((struct CPSL_List*)List)->End) );
}

template <typename T>
static inline T CPSL_LNEXT(T List)
{
	return reinterpret_cast<T>( (((struct CPSL_List*)List)->Next) );
}

template <typename T>
static inline T CPSL_LPREV(T List)
{
	return reinterpret_cast<T>( (((struct CPSL_List*)List)->Prev) );
}

#else /*If we are C, we use a simple macro instead of a template.*/

#define CPSL_LNEXT(x) ((void*)((struct CPSL_List*)x)->Next)
#define CPSL_LPREV(x) ((void*)((struct CPSL_List*)x)->Prev)
#define CPSL_LHEAD(x) ((void*)*((struct CPSL_List*)x)->Head)
#define CPSL_LEND(x)  ((void*)*((struct CPSL_List*)x)->End)

#endif /*__cplusplus*/

#endif /*__LIBCPSL_H__*/

