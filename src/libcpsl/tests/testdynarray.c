#include "test_header.h"

void TestInitialization(void)
{
	int Array[3] = { 1, 2, 3 };
	
	int *DynArray = CPSL_DynArray_New_Inz(sizeof(int), 100, Array, sizeof Array / sizeof *Array);
	
	if (Array[0] != 1 || Array[1] != 2 || Array[2] != 3)
	{
		FAILTEST("Initialization failed, there appears to be memory corruption or copying failure.");
	}
	
	if (CPSL_DynArray_Capacity(DynArray) != 100)
	{
		FAILTEST("Capacity does not match the requested allocated size.");
	}
	
	if (CPSL_DynArray_ElementSize(DynArray) != sizeof(int))
	{
		FAILTEST("Element size is not correct.");
	}
	
	for (int Inc = 0; Inc < CPSL_DynArray_Capacity(DynArray); ++Inc)
	{
		DynArray[Inc] = 0;
		int Gerbil = DynArray[Inc];
		(void)Gerbil;
	}
	CPSL_DynArray_Destroy(DynArray);
}

void TestNoInitializer(void)
{
	int *DynArray = CPSL_DynArray_New(sizeof(int), 150);
	
	if (CPSL_DynArray_Capacity(DynArray) != 150)
	{
		FAILTEST("Capacity does not match the requested allocated size.");
	}
	
	if (CPSL_DynArray_ElementSize(DynArray) != sizeof(int))
	{
		FAILTEST("Element size is not correct.");
	}
	
	for (int Inc = 0; Inc < CPSL_DynArray_Capacity(DynArray); ++Inc)
	{
		DynArray[Inc] = 0;
		int Gerbil = DynArray[Inc];
		(void)Gerbil;
	}
	
	CPSL_DynArray_Destroy(DynArray);
}

void TestResize(void)
{
	int *DynArray = CPSL_DynArray_New(sizeof(int), 150);
	
	DynArray = CPSL_DynArray_Grow(DynArray, 100);
	
	if (CPSL_DynArray_Capacity(DynArray) != 250)
	{
		FAILTEST("Grow test failed to allocate to the correct size, or is returning bad data.");
	}
	
	for (int Inc = 0; Inc < CPSL_DynArray_Capacity(DynArray); ++Inc)
	{
		DynArray[Inc] = 0;
		int Gerbil = DynArray[Inc];
		(void)Gerbil;
	}
	
	DynArray = CPSL_DynArray_Shrink(DynArray, 200);
	
	if (CPSL_DynArray_Capacity(DynArray) != 50)
	{
		FAILTEST("Shrink test failed to properly shrink the capacity to the correct size, or is returning bad data.");
	}
	
	for (int Inc = 0; Inc < CPSL_DynArray_Capacity(DynArray); ++Inc)
	{
		DynArray[Inc] = 0;
		int Gerbil = DynArray[Inc];
		(void)Gerbil;
	}
	
	//Try without realloc
	CPSL_Configure(malloc, free, NULL);
	
	DynArray = CPSL_DynArray_Grow(DynArray, 25);
	
	if (CPSL_DynArray_Capacity(DynArray) != 75)
	{
		FAILTEST("Grow test without realloc failed.");
	}
	CPSL_Configure(malloc, free, realloc);
	
	for (int Inc = 0; Inc < CPSL_DynArray_Capacity(DynArray); ++Inc)
	{
		DynArray[Inc] = 0;
		int Gerbil = DynArray[Inc];
		(void)Gerbil;
	}
	
	CPSL_DynArray_Destroy(DynArray);
}
	
int main(void)
{
	InitTestUnit("dynamic arrays");
	
	RUNTEST(TestInitialization);
	RUNTEST(TestNoInitializer);
	RUNTEST(TestResize);
}
