#pragma once

#include "CAnimal.h"
#include <vector>

class CResMgr
{
public:
	~CResMgr();
	void Register(CAnimal*& pAnimal);
private:
	vector<CAnimal*> m_resVct;
};

