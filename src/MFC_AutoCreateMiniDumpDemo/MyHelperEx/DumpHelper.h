#pragma once

#include <minidumpapiset.h>
#include <strsafe.h>
#include <mutex>
#include <intsafe.h>

#ifdef _USRDLL

#define CppProject_Export_Ex __declspec(dllexport)

#else

#define CppProject_Export_Ex __declspec(dllimport)

#endif

//VOID CreateDump(struct _EXCEPTION_POINTERS* pExceptionPointers);

//LONG WINAPI DumpCallback(_EXCEPTION_POINTERS* excp);

extern "C" void CppProject_Export_Ex SetUnhandledExceptionFilterEx();
