#include <iostream>
#include <CDog.h>
#include <CCat.h>
#include <CResMgr.h>
#include "ConApp_res_manager.h"

int main()
{

	try
	{
		Test();
	}
	/*catch (CMemoryException* e)
	{

	}
	catch (CFileException* e)
	{
	}
	catch (CException* e)
	{
	}*/
	catch (...)
	{
		//在这句信息打印之前，指针已得到有效释放
		cout << "before show error... , pDog , pCat pointers had released (depends by CResMgr instance)..." << endl;
		cout << "show error ..." << endl;
	}

	return 0;
}

void Test()
{
	CResMgr mgr;

	CAnimal* pDog = new CDog();
	CAnimal* pCat = new CCat();

	if (pDog != nullptr)
	{
		mgr.Register(pDog);
		pDog->Run();
	}

	if (pCat != nullptr)
	{
		mgr.Register(pCat);
		pCat->Run();
	}


	cout << "===========================================" << endl;
	throw "... error ...";//模拟在正常释放指针前 异常

	delete pDog;
	pDog = nullptr;

	delete pCat;
	pCat = nullptr;
}
