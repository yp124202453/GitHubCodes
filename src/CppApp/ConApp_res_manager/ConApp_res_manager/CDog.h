#pragma once
#include "CAnimal.h"
class CDog :
	public CAnimal
{
public:
	~CDog();

	void Run() override;
};

