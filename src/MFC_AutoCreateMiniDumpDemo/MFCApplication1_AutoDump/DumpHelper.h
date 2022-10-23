#pragma once

#include <minidumpapiset.h>
#include <strsafe.h>
#include <mutex>
#include <intsafe.h>

//VOID CreateDump(struct _EXCEPTION_POINTERS* pExceptionPointers);

extern LONG WINAPI DumpCallback(_EXCEPTION_POINTERS* excp);

