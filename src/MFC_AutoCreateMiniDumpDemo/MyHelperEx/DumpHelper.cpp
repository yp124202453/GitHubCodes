#include "pch.h"
#include "DumpHelper.h"

#pragma comment(lib, "Dbghelp.lib") 

using namespace std;

void CreateDumpW(struct _EXCEPTION_POINTERS* pExceptionPointers)
{

	WCHAR pBuffer[1024] = { 0 };
	swprintf_s(pBuffer, 1024, L"Build: %hs %hs", __DATE__, __TIME__);
	wstring strBuild(pBuffer);

	swprintf_s(pBuffer, 1024, L"my_module_name %d , %d , %p .", pExceptionPointers->ExceptionRecord->ExceptionCode,
		pExceptionPointers->ExceptionRecord->ExceptionFlags, pExceptionPointers->ExceptionRecord->ExceptionAddress);
	wstring strError(pBuffer);

	//Éú³É mini crash dump
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

LONG WINAPI DumpCallback(_EXCEPTION_POINTERS* excp)
{
	mutex mtx;
	mtx.lock();

	CreateDumpW(excp);

	mtx.unlock();
	return EXCEPTION_EXECUTE_HANDLER;
}

void CppProject_Export_Ex SetUnhandledExceptionFilterEx()
{
	SetUnhandledExceptionFilter(DumpCallback);
}
