#include <io.h>
#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

bool transfer(string fileName, int exeNum);
void dfsFolder(string folderPath, ofstream& fout);
void GetAllDirecories(const string& folderPath, vector<string>& vctDirs, bool bContainsSubDirectory = false);

int main(int argc, const char** argv)
{
	string strPath(argv[0]);
	size_t iIndex = strPath.find_last_of('\\');
	if (iIndex == string::npos)
	{
		return 0;
	}

	strPath = strPath.substr(0, iIndex);
	strPath += "\\*.*";
	_finddata_t file;
	int k;

	intptr_t  HANDLE;

	//k = HANDLE = _findfirst("*.*", &file);
	k = HANDLE = _findfirst(strPath.c_str(), &file);
	while (k != -1)
	{
		cout << file.name << endl;
		k = _findnext(HANDLE, &file);
	}
	_findclose(HANDLE);

	//transfer("D:\\Demo\\Cpp\\ConApp\\ConsoleApplication1_FindAllDirs\\Debug\\*.exe", 0);

	cout << endl << endl;

	/*ofstream o_fstream;
	dfsFolder("D:\\Demo\\Cpp\\ConApp\\ConsoleApplication1_FindAllDirs\\Debug", o_fstream);*/

	vector<string> vctDirs;
	GetAllDirecories("D:\\Demo\\Cpp\\ConApp\\ConsoleApplication1_FindAllDirs\\x64", vctDirs);
	cout << "total diretories count = " << vctDirs.size() << endl;

	cout << endl << endl;
	vector<string> vctDirs2;
	GetAllDirecories("D:\\Demo\\Cpp\\ConApp\\ConsoleApplication1_FindAllDirs\\x64\\", vctDirs2, true);
	cout << "total diretories count = " << vctDirs2.size() << endl;

	return 0;
}

//_findfirst 函数返回的是匹配到文件的句柄，数据类型为long。
//遍历过程可以指定文件类型，这通过FileName的赋值来实现，例如要遍历C : \WINDOWS下的所有.exe文件
bool transfer(string fileName, int exeNum)
{
	_finddata_t fileInfo;
	intptr_t handle = _findfirst(fileName.c_str(), &fileInfo);

	if (handle == -1L)
	{
		cerr << "failed to transfer files" << endl;
		return false;
	}

	do
	{
		exeNum++;
		cout << fileInfo.name << endl;
	}
	while (_findnext(handle, &fileInfo) == 0);
	cout << " .exe files' number:  " << exeNum << endl;

	return true;
}

//遍历文件夹及其子文件夹下所有文件。操作系统中文件夹目录是树状结构，使用深度搜索策略遍历所有文件。用到_A_SUBDIR属性
//在判断有无子目录的if分支中，由于系统在进入一个子目录时，匹配到的头两个文件(夹)是"."(当前目录)，".."(上一层目录)。
//需要忽略掉这两种情况。当需要对遍历到的文件做处理时，在else分支中添加相应的代码就好
void dfsFolder(string folderPath, ofstream& fout)
{
	_finddata_t FileInfo;
	string strfind = folderPath + "\\*";
	intptr_t Handle = _findfirst(strfind.c_str(), &FileInfo);

	if (Handle == -1L)
	{
		cerr << "can not match the folder path" << endl;
		exit(-1);
	}
	do
	{
		//判断是否有子目录  
		if (FileInfo.attrib & _A_SUBDIR)
		{
			//这个语句很重要  
			if ((strcmp(FileInfo.name, ".") != 0) && (strcmp(FileInfo.name, "..") != 0))
			{
				string newPath = folderPath + "\\" + FileInfo.name;
				cout << folderPath.c_str() << "\\" << FileInfo.name << endl;
				dfsFolder(newPath, fout);
			}
		}
		else
		{
			fout << folderPath.c_str() << "\\" << FileInfo.name << " ";
			cout << folderPath.c_str() << "\\" << FileInfo.name << endl;
		}
	}
	while (_findnext(Handle, &FileInfo) == 0);

	_findclose(Handle);
	fout.close();
}

/// <summary>
/// 递归获取指定的文件夹路径 strRootDirPath 中的 所有文件夹（不包括它自己）
/// </summary>
/// <param name="strRootDirPath">指定的文件夹路径</param>
/// <param name="vctDirs">存放获取到的文件夹路径(入参传入前，需要清空vct)</param>
/// <param name="bContainsSubDirectory">是否包括子文件夹</param>
void GetAllDirecories(const string& strRootDirPath, vector<string>& vctDirs, bool bContainsSubDirectory)
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
				cout << strDirPath.c_str() << endl;

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