#include "CResMgr.h"

CResMgr::~CResMgr()
{
	for (auto& ptr : m_resVct)
	{
		delete ptr;
	}

	m_resVct.clear();
}

void CResMgr::Register(CAnimal*& pAnimal)
{
	if (pAnimal != nullptr)
	{
		m_resVct.push_back(pAnimal);
	}
}
