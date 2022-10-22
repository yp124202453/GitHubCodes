#include "pch.h"
#include "CTestMath.h"
#include "TestDLLMath.h"

int CTestMath::Add(int a, int b)
{
	return a + b;
}

int CTestMath::Sub(int a, int b)
{
	return a - b;
}

int Add(int a, int b)
{
	CTestMath item;
	return item.Add(a, b);
}

int Sub(int a, int b)
{
	CTestMath item;
	return item.Sub(a, b);
}
