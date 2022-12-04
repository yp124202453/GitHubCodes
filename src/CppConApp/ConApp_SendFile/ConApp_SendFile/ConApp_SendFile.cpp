#include <iostream>
#include <fstream>
#include <string>
#include "ConApp_SendFile.h"
#include "SerialPort.h"


using namespace std;

//设置内存对齐长度为 2
#pragma pack(2)

typedef struct _MyStruct
{
	char ch;
	int i;
	short s;
	char buffer[32];
}MyStruct;

void GetFileSize(const std::string& strFile, unsigned int& fileSize)
{
	ifstream inStream(strFile, std::ios_base::in | std::ios_base::binary);
	streampos  iStart = inStream.tellg();
	inStream.seekg(0, ios::end);
	streampos iEnd = inStream.tellg();
	inStream.close();
	fileSize = (unsigned int)iEnd;
}

string GetDateTime()
{
	SYSTEMTIME st = { 0 };
	GetLocalTime(&st);
	char buf[32] = { 0 };
	sprintf_s(buf, 32, "%04d-%02d-%02d_%02d-%02d-%02d_%03d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	string str(buf);
	return str;
}

int main()
{
	cout << "Hello World!\n";

	cout << "alignof(MyStruct) = " << alignof(MyStruct) << endl;
	cout << "alignof(std::max_align_t) =" << alignof(std::max_align_t) << endl;
	unsigned int fileSize = 0;

	/*string strFile = "D:\\GitHubCodes\\src\\DisAssembly\\Codes\\ConApp_SendFile\\ConApp_SendFile\\SaveWindows2022_12_3_19-08-59.txt";
	GetFileSize(strFile, fileSize);*/

	string strDateTime = GetDateTime();
	cout << strDateTime << endl;

	char szPath[_MAX_PATH] = { 0 };
	GetModuleFileNameA(NULL, szPath, sizeof(szPath) - 1);
	string strAppDir;
	string strAppPath(szPath);
	size_t index = strAppPath.find_last_of('\\');
	if (index == string::npos)
	{
		cout << "get app dir error" << endl;
		return -1;
	}
	else
	{
		strAppDir = strAppPath.substr(0, index);
	}

	//string strNewFilePath = strAppDir + "\\" + strDateTime + ".txt";
	string strNewFilePath = strAppDir + "\\" + strDateTime + ".zip";
	cout << strNewFilePath << endl;

	CSerialPort* m_pCom = new CSerialPort();
	if (nullptr == m_pCom)
	{
		cout << "new CSerialPort error" << endl;
		return -1;
	}

	const int bufSize = 1050;
	int readBytesCount = 0;
	byte* buf = new byte[bufSize];
	m_pCom->InitPort(3, 115200, 'N', 8, 1, EV_RXCHAR);
	FILE* fp = nullptr;
	fopen_s(&fp, strNewFilePath.c_str(), "wb");
	if (nullptr == fp)
	{
		cout << "create FILE* error" << endl;
		return -1;
	}

	cout << "start write file" << endl;
	byte ackArr[3] = { 0x31,0x32,0x33 };
	int writeCount = 0;
	int totalBytesCount = 0;
	int packCount = 0;

	while (true)
	{
		readBytesCount = m_pCom->ReadBytes(buf, bufSize);
		if (readBytesCount < 1) { continue; }

		if (readBytesCount == 3)
		{
			m_pCom->WriteData(ackArr, 3);
			//456 为结束发送文件
			if (buf[0] == 0x34
				&& buf[1] == 0x35
				&& buf[2] == 0x36)
			{
				break;
			}
			else
			{
				continue;
			}
		}
		else if (readBytesCount > 3)
		{
			totalBytesCount += readBytesCount;
			fwrite(buf, 1, readBytesCount, fp);
			if ((++writeCount) == 20)
			{
				fflush(fp);
				writeCount = 0;
			}

			++packCount;
			cout << "recv file package count = " << packCount << endl;
			m_pCom->WriteData(ackArr, 3);
		}
		else
		{
			m_pCom->WriteData(ackArr, 3);
		}
	}

	fflush(fp);
	fclose(fp);

	cout << "end write file : status success" << endl;
	cout << "file size = " << totalBytesCount << " bytes" << endl;
	system("pause");
	return 0;
}

