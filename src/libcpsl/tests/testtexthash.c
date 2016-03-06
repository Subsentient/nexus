#include "test_header.h"



void TestHashLookup(void)
{
	CPSL_Hash Hash = CPSL_Hash_New();
	
	int Gerbil1 = 777;
	int Gerbil2 = 444;

	CPSL_Hash_Set(Hash, "One", &Gerbil1, sizeof Gerbil1);
	CPSL_Hash_Set(Hash, "Two", &Gerbil2, sizeof Gerbil2);
	
	int GerbilNew1 = 0;
	int GerbilNew2 = 0;
	struct CPSL_HashElement *Element1 = CPSL_Hash_Get(Hash, "One", &GerbilNew1);
	struct CPSL_HashElement *Element2 = CPSL_Hash_Get(Hash, "Two", &GerbilNew2);
	
	if (!Element1 || !Element2)
	{
		FAILTEST("Failed to lookup recently created element!");
	}
	
	if (GerbilNew1 != 777 || GerbilNew2 != 444)
	{
		FAILTEST("Retrieved values from hash do not match!");
	}
	
	CPSL_Hash_Set(Hash, "One", NULL, 0); //Delete one.
	CPSL_Hash_Set(Hash, "Three", &Gerbil1, sizeof Gerbil1);//Randomly set another one to test durability.
	CPSL_Hash_Set(Hash, "Two", NULL, 0); //Delete two.
	
	if (CPSL_Hash_Get(Hash, "One", NULL) != NULL || CPSL_Hash_Get(Hash, "Two", NULL) != NULL)
	{
		FAILTEST("Node not deleted properly!");
	}
	
	if (!CPSL_Hash_Get(Hash, "Three", NULL))
	{
		FAILTEST("Destruction of other nodes resulted in destruction of one we wanted to keep!");
	}
	
	CPSL_Hash_Destroy(Hash);
}

int main(void)
{
	InitTestUnit("text hashes");
	
	RUNTEST(TestHashLookup);
	return 0;
}
