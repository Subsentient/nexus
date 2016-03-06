#include "test_header.h"

void TestRaw(void)
{
	//Test a raw list with nothing assigned to any of it.
	struct CPSL_List *List = CPSL_List_NewList(sizeof(struct CPSL_List));
	
	List->Data = (void*)1;
	
	for (int Inc = 0; Inc < 50; ++Inc)
	{
		((List = CPSL_List_AddNode(List)))->Data = (char*)Inc + 2;
	}
	
	struct CPSL_List *Worker = CPSL_LHEAD(List);
	
	//Forward.
	for (; Worker; Worker = CPSL_LNEXT(Worker));
	
	
	Worker = CPSL_LEND(List);

	//Backward.
	for (; Worker; Worker = CPSL_LPREV(Worker));
	
	Worker = List->Next;
	
	while ((Worker = CPSL_List_DeleteNode(Worker)));
	
}

void TestChunky(void)
{
	struct ChunkyStruct
	{
		struct CPSL_List List;
		int Farts;
	};

	struct ChunkyStruct *List = (void*)CPSL_List_NewList(sizeof(struct ChunkyStruct));
	
	for (int Inc = 0; Inc < 50; ++Inc)
	{
		((List = (void*)CPSL_List_AddNode((void*)List)))->Farts = Inc + 1;
	}
	
	struct ChunkyStruct *Worker = CPSL_LHEAD(List);
	
	//Forward.
	for (; Worker; Worker = CPSL_LNEXT(Worker));
	
	Worker = CPSL_LEND(List);

	//Backward.
	for (; Worker; Worker = CPSL_LPREV(Worker));
	
	Worker = CPSL_LNEXT(List);
	
	while ((Worker = (struct ChunkyStruct*)CPSL_List_DeleteNode((struct CPSL_List*)Worker)));
}

int main(void)
{
	//Initialization function.
	InitTestUnit("linked lists");
	
	RUNTEST(TestRaw);
	RUNTEST(TestChunky);
	return 0;
}
