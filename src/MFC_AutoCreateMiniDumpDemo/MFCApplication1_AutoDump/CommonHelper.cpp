#include "pch.h"
#include "CommonHelper.h"
#include <corecrt_io.h>

/// <summary>
/// �ݹ��ȡָ�����ļ���·�� strRootDirPath �е� �����ļ��У����������Լ���
/// </summary>
/// <param name="strRootDirPath">ָ�����ļ���·��</param>
/// <param name="vctDirs">��Ż�ȡ�����ļ���·��(��δ���ǰ����Ҫ���vct)</param>
/// <param name="bContainsSubDirectory">�Ƿ�������ļ���</param>
void CommonHelper::GetAllDirecories(const string& strRootDirPath, vector<string>& vctDirs, bool bContainsSubDirectory)
{
	if (strRootDirPath.empty()) { return; }

	//�ж��ļ����Ƿ���� 
	if (_access(strRootDirPath.c_str(), 0) == -1)
	{
		//_mkdir(strRootDir.c_str());//�����ļ���
		return;
	}

	_finddata_t FileInfo;
	string strfind;

	//·���� \ ��β
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
		//�ж��Ƿ�����Ŀ¼  
		if (FileInfo.attrib & _A_SUBDIR)
		{
			//���������Ҫ  
			if ((strcmp(FileInfo.name, ".") != 0) && (strcmp(FileInfo.name, "..") != 0))
			{
				string strDirPath = strRootDirPath + "\\" + FileInfo.name;
				vctDirs.emplace_back(strDirPath);

				//�ݹ����ļ���
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