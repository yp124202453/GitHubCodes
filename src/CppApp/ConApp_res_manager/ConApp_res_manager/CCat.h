#pragma once
#include "CAnimal.h"
class CCat :
	public CAnimal
{
public:
	~CCat();
	void Run() override;
};

