#pragma once
#include <iostream>

using namespace std;

class CAnimal
{
public:
	virtual ~CAnimal();

	virtual void Run() = 0;
};

