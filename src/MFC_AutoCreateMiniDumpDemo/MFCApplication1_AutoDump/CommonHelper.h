#pragma once
#include <vector>
#include <string>

using namespace std;

class CommonHelper
{
public:

	static void GetAllDirecories(const string& strRootDirPath, vector<string>& vctDirs, bool bContainsSubDirectory);

};

