#include "pch.h"
#include "DumpHelper.h"
//#include <minidumpapiset.h>

#pragma comment(lib, "Dbghelp.lib") 

using namespace std;

VOID CreateDumpW(struct _EXCEPTION_POINTERS* pExceptionPointers);

VOID CreateDump(struct _EXCEPTION_POINTERS* pExceptionPointers)
{
	//收集信息
	CStringW strBuild;
	strBuild.Format(L"Build: %hs %hs", __DATE__, __TIME__);

	CStringW strError;
	WCHAR* szModuleName = L"my_module_name";

	strError.Format(L"%s %d, %d, %p.", szModuleName, pExceptionPointers->ExceptionRecord->ExceptionCode,
		pExceptionPointers->ExceptionRecord->ExceptionFlags, pExceptionPointers->ExceptionRecord->ExceptionAddress);

	//生成 mini crash dump
	BOOL bMiniDumpSuccessful;
	WCHAR* szPath = L"./";
	WCHAR szFileName[MAX_PATH];
	WCHAR* szAppName = L"DumpFile";
	WCHAR* szVersion = L"v1.0";
	DWORD dwBufferSize = MAX_PATH;
	HANDLE hDumpFile;
	SYSTEMTIME stLocalTime;
	MINIDUMP_EXCEPTION_INFORMATION ExpParam;
	GetLocalTime(&stLocalTime);
	//GetTempPathW(dwBufferSize, szPath);
	StringCchPrintfW(szFileName, MAX_PATH, L"%s%s", szPath, szAppName);
	CreateDirectoryW(szFileName, NULL);
	//std::wcout << szFileName;
	StringCchPrintfW(szFileName, MAX_PATH, L"%s%s//%s-%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp",
		szPath, szAppName, szVersion,
		stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
		stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond,
		GetCurrentProcessId(), GetCurrentThreadId());
	hDumpFile = CreateFileW(szFileName, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

	MINIDUMP_USER_STREAM UserStream[2];
	MINIDUMP_USER_STREAM_INFORMATION UserInfo;
	UserInfo.UserStreamCount = 1;
	UserInfo.UserStreamArray = UserStream;
	UserStream[0].Type = CommentStreamW;
	UserStream[0].BufferSize = strBuild.GetLength() * sizeof(WCHAR);
	UserStream[0].Buffer = strBuild.GetBuffer();
	UserStream[1].Type = CommentStreamW;
	UserStream[1].BufferSize = strError.GetLength() * sizeof(WCHAR);
	UserStream[1].Buffer = strError.GetBuffer();

	ExpParam.ThreadId = GetCurrentThreadId();
	ExpParam.ExceptionPointers = pExceptionPointers;
	ExpParam.ClientPointers = TRUE;

	MINIDUMP_TYPE MiniDumpWithDataSegs = (MINIDUMP_TYPE)(MiniDumpNormal
		| MiniDumpWithHandleData
		| MiniDumpWithUnloadedModules
		| MiniDumpWithIndirectlyReferencedMemory
		| MiniDumpScanMemory
		| MiniDumpWithProcessThreadData
		| MiniDumpWithThreadInfo);

	bMiniDumpSuccessful = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
		hDumpFile, MiniDumpWithDataSegs, &ExpParam, NULL, NULL);

	return;
}

LONG WINAPI DumpCallback(_EXCEPTION_POINTERS* excp)
{
	/*boost::mutex::scoped_lock lock(g_dump_mutex);*/
	mutex mtx;
	mtx.lock();

	CreateDumpW(excp);

	mtx.unlock();
	return EXCEPTION_EXECUTE_HANDLER;
}

void CreateDumpW(struct _EXCEPTION_POINTERS* pExceptionPointers)
{

	WCHAR pBuffer[1024] = { 0 };
	swprintf_s(pBuffer, 1024, L"Build: %hs %hs", __DATE__, __TIME__);
	wstring strBuild(pBuffer);

	swprintf_s(pBuffer, 1024, L"my_module_name %d , %d , %p .", pExceptionPointers->ExceptionRecord->ExceptionCode,
		pExceptionPointers->ExceptionRecord->ExceptionFlags, pExceptionPointers->ExceptionRecord->ExceptionAddress);
	wstring strError(pBuffer);

	//生成 mini crash dump
	BOOL bMiniDumpSuccessful;
	const WCHAR* szPath = L"./";
	WCHAR szFileName[MAX_PATH] = { 0 };
	const WCHAR* szAppName = L"DumpFile";
	/*const WCHAR* szVersion = L"v1.0";*/
	DWORD dwBufferSize = MAX_PATH;
	HANDLE hDumpFile;
	SYSTEMTIME stLocalTime;
	MINIDUMP_EXCEPTION_INFORMATION ExpParam;
	GetLocalTime(&stLocalTime);
	swprintf_s(szFileName, MAX_PATH, L"%s%s", szPath, szAppName);

	CreateDirectory(szFileName, NULL);

	StringCchPrintfW(szFileName, MAX_PATH, L"%s%s//%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp",
		szPath, szAppName,
		stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
		stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond,
		GetCurrentProcessId(), GetCurrentThreadId());
	hDumpFile = CreateFileW(szFileName, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

	MINIDUMP_USER_STREAM UserStream[2];
	MINIDUMP_USER_STREAM_INFORMATION UserInfo;
	UserInfo.UserStreamCount = 1;
	UserInfo.UserStreamArray = UserStream;
	UserStream[0].Type = CommentStreamW;
	UserStream[0].BufferSize = strBuild.size() * sizeof(WCHAR);
	UserStream[0].Buffer = (PVOID)(strBuild.c_str());

	UserStream[1].Type = CommentStreamW;
	UserStream[1].BufferSize = strError.size() * sizeof(WCHAR);
	UserStream[1].Buffer = (PVOID)(strError.c_str());

	ExpParam.ThreadId = GetCurrentThreadId();
	ExpParam.ExceptionPointers = pExceptionPointers;
	ExpParam.ClientPointers = TRUE;

	MINIDUMP_TYPE MiniDumpWithDataSegs = (MINIDUMP_TYPE)(MiniDumpNormal
		| MiniDumpWithHandleData
		| MiniDumpWithUnloadedModules
		| MiniDumpWithIndirectlyReferencedMemory
		| MiniDumpScanMemory
		| MiniDumpWithProcessThreadData
		| MiniDumpWithThreadInfo);

	bMiniDumpSuccessful = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
		hDumpFile, MiniDumpWithDataSegs, &ExpParam, NULL, NULL);

	return;
}