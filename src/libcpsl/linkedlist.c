/*This file is part of libcpsl.
 * libcpsl is public domain software; you may use it however you please.
 * Refer to the file UNLICENSE.txt for more information.
 * 2016, by Subsentient, the white rat hiding under your chair. Don't look.
 */

///I don't feel like using C89 for the entire thing, but I'll make sure the public header file doesn't contain any C99 stuff.


#include "libcpsl.h"
#include "cpslinternal.h"

struct CPSL_List *CPSL_List_NewList(const unsigned PerElementSize)
{	
	if (PerElementSize < sizeof(struct CPSL_List)) return NULL;
	
	struct CPSL_List *Core = Alloc.malloc(PerElementSize);
	Core->Data = NULL;
	Core->Next = NULL;
	Core->Prev = NULL;

	
	/*Pointer to a pointer to the end and head, respectively, of the list.
	* The reason we use a pointer to pointer is that End and possibly Head will change frequently and we
	* don't want to iterate through the whole list to change it for each element.*/
	Core->End = Alloc.malloc(sizeof(struct CPSL_List*));
	*Core->End = Core;
	Core->Head = Alloc.malloc(sizeof(struct CPSL_List*));
	*Core->Head = Core;
	
	//Pointer to a single integer that contains the per-object size.
	Core->PerElementSize = Alloc.malloc(sizeof(unsigned));
	*(unsigned*)Core->PerElementSize = PerElementSize;
	
	return Core;
}


CPSL_Bool CPSL_List_DestroyList(struct CPSL_List *ListHead)
{
	//Null pointer?
	if (!ListHead) return false;
	
	
	//In case we were passed a later element and not the head of the list.
	ListHead = *ListHead->Head;
	
	struct CPSL_List *Worker = ListHead;
	
	struct CPSL_List *Del = NULL;
	
	//Release the pointers to head and end.
	Alloc.free(ListHead->End);
	Alloc.free(ListHead->Head);
	
	//Release the pointer to the per-object size.
	Alloc.free((void*)ListHead->PerElementSize);
	
	for (; Worker; Worker = Del)
	{
		Del = Worker->Next;
		Alloc.free(Worker);
	}
	
	return true;
}

struct CPSL_List *CPSL_List_AddNode(struct CPSL_List *ListElement)
{
	if (!ListElement)
	{
		return NULL;
	}
	

	//In case this isn't the list head.
	ListElement = *ListElement->Head;
	
	struct CPSL_List *NewNode = Alloc.malloc(*ListElement->PerElementSize);
	
	NewNode->Data = NULL; //Keep it empty for the user.
	NewNode->Next = NULL; //Nothing after us.
	NewNode->Prev = *ListElement->End; //Since we're now the end node, the previous end node is our Prev.
	
	//Tell the old end node that we're now the end node.
	(*ListElement->End)->Next = NewNode;
	
	//We must remember our starting and ending places.
	NewNode->Head = ListElement->Head;
	NewNode->End = ListElement->End;
	//And we must remember how big we are each.
	NewNode->PerElementSize = ListElement->PerElementSize;
	
	//We are the end node now;
	*NewNode->End = NewNode;
	
	return NewNode;
}

struct CPSL_List *CPSL_List_DeleteNode(struct CPSL_List *NodeToDelete)
{ //Note that we use the ->Head member to figure out which list it's from.
	if (!NodeToDelete) return NULL; //You gotta be pretty dumb to pass us a null pointer.

	if (NodeToDelete == *NodeToDelete->Head)
	{
		if (NodeToDelete->Next) //We're not alone
		{
			//Move the head.
			*NodeToDelete->Head = NodeToDelete->Next;
			NodeToDelete->Next->Prev = NULL;
			Alloc.free(NodeToDelete);
			return NodeToDelete->Next; //Give them the new head.
		}

		//Ahh, just us. So the list dies now.
		CPSL_List_DestroyList(NodeToDelete);
		return NULL; //Tells them the list is gone.
	}
	
	struct CPSL_List *const RetVal = *NodeToDelete->Head;
	
	if (NodeToDelete == *NodeToDelete->End)
	{
		*NodeToDelete->End = NodeToDelete->Prev;
		NodeToDelete->Prev->Next = NULL;
	}
	else
	{
		NodeToDelete->Prev->Next = NodeToDelete->Next;
		NodeToDelete->Next->Prev = NodeToDelete->Prev;
	}
	
	Alloc.free(NodeToDelete);
	return RetVal;
}
