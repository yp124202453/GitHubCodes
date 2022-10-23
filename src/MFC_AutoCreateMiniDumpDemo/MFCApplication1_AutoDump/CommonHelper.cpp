#include "pch.h"
#include "CommonHelper.h"
#include <corecrt_io.h>

/// <summary>
/// 递归获取指定的文件夹路径 strRootDirPath 中的 所有文件夹（不包括它自己）
/// </summary>
/// <param name="strRootDirPath">指定的文件夹路径</param>
/// <param name="vctDirs">存放获取到的文件夹路径(入参传入前，需要清空vct)</param>
/// <param name="bContainsSubDirectory">是否包括子文件夹</param>
void CommonHelper::GetAllDirecories(const string& strRootDirPath, vector<string>& vctDirs, bool bContainsSubDirectory)
{
	if (strRootDirPath.empty()) { return; }

	//判断文件夹是否存在 
	if (_access(strRootDirPath.c_str(), 0) == -1)
	{
		//_mkdir(strRootDir.c_str());//创建文件夹
		return;
	}

	_finddata_t FileInfo;
	string strfind;

	//路径以 \ 结尾
	if (strRootDirPath.find_last_of('\\') == (strRootDirPath.size() - 1))
	{
		strfind = strRootDirPath + "*";
	}
	else
	{
		strfind = strRootDirPath + "\\*";
	}

	intptr_t Handle = _findfirst(strfind.c_str(), &FileInfo);
	if (Handle == -1)
	{
		return;
	}

	do
	{
		//判断是否有子目录  
		if (FileInfo.attrib & _A_SUBDIR)
		{
			//这个语句很重要  
			if ((strcmp(FileInfo.name, ".") != 0) && (strcmp(FileInfo.name, "..") != 0))
			{
				string strDirPath = strRootDirPath + "\\" + FileInfo.name;
				vctDirs.emplace_back(strDirPath);

				//递归子文件夹
				if (bContainsSubDirectory)
				{
					GetAllDirecories(strDirPath, vctDirs, bContainsSubDirectory);
				}
			}
		}
		/*else
		{
			cout << strDirPath.c_str() << endl;
		}*/
	}
	while (_findnext(Handle, &FileInfo) == 0);

	_findclose(Handle);
}