// terminate-hook.cpp
//
// 用 Microsoft Detours 内联 hook std::terminate / __std_terminate，
// 把 C++ 层的 terminate 路径（noexcept 违例、std::thread 未捕获异常、
// 显式 std::terminate()）改写成 RaiseException 抛出 SEH 异常，
// 使其流经 UEH (SetUnhandledExceptionFilter / WER)，
// 便于崩溃上报系统（WER、LocalDumps、第三方 crash handler）统一采集。
//
// 编译: vcpkg (manifest) + MSVC，依赖 detours。

#include <Windows.h>
#include <Dbghelp.h>
#include <detours/detours.h>

#include <exception>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <stdexcept>
#include <thread>

#pragma comment(lib, "dbghelp.lib")

// ---------------------------------------------------------------------------
// msvcp140.dll 的 ABI 入口：MSVC 编译器在 noexcept 违例时直接调用它，
// 不保证经过 std::terminate()。因此两个入口都要 hook。
// __std_terminate 由 <exception>/<yvals.h> 标准头声明，签名以头文件为准。
// ---------------------------------------------------------------------------

// 自定义异常码: 0xE000xxxx = customer + severity error，标记为不可继续
static constexpr DWORD kTerminateExceptionCode = 0xE0005443; // "TC" = Terminate->Crash

using TerminateFn = void (*)();
static TerminateFn g_realTerminate = std::terminate;
static void(__stdcall* g_realStdTerminate)() noexcept = __std_terminate;

// 供 UEH 读取的“终止原因”
static char g_lastReason[512] = {};

// 显式 std::terminate() 的退出码
static constexpr UINT kExplicitTerminateExitCode = 1;

// ---------------------------------------------------------------------------
// terminate 被拦截时，把仍在飞行中的异常对象取出来（rethrow 到 catch），
// 记录 what() 信息，然后抛 SEH 异常走 UEH。
// ---------------------------------------------------------------------------
static void CaptureCurrentException()
{
	try
	{
		if (std::current_exception())
		{
			std::rethrow_exception(std::current_exception());
		}
	}
	catch (const std::exception& e)
	{
		_snprintf_s(g_lastReason, _TRUNCATE, "std::exception: %s", e.what());
		return;
	}
	catch (...)
	{
		_snprintf_s(g_lastReason, _TRUNCATE, "non-standard exception object");
		return;
	}
	_snprintf_s(g_lastReason, _TRUNCATE, "terminate called without active exception");
}

// ---------------------------------------------------------------------------
// Detour: 替代原 terminate —— 不再走 abort()，而是 RaiseException
// ---------------------------------------------------------------------------
static void HookedTerminate() noexcept
{
	CaptureCurrentException();

	// 无活动异常 => 用户代码显式调用 std::terminate()：
	// 不生成 dump、不走 UEH，直接退出进程
	if (std::strcmp(g_lastReason, "terminate called without active exception") == 0)
	{
		fprintf(stderr, "[hook] explicit std::terminate() (no active exception) -> no dump, exiting process directly\n");
		fflush(stderr);
		ExitProcess(kExplicitTerminateExitCode);
	}

	fprintf(stderr, "[hook] terminate intercepted! reason: %s\n", g_lastReason);
	fprintf(stderr, "[hook] raising SEH exception 0x%08X -> dispatching to UEH...\n",
		kTerminateExceptionCode);
	fflush(stderr);

	// NONCONTINUABLE + 无 SEH handler => 异常分发交由 UEH / WER 处理，
	// 本函数永不返回（进程在此路径上结束）。
	RaiseException(kTerminateExceptionCode, EXCEPTION_NONCONTINUABLE, 0, nullptr);

	// 理论上不可达；防御性兜底
	for (;;)
	{
		Sleep(1000);
	}
}

// ---------------------------------------------------------------------------
// MiniDump 生成：写到 exe 目录\DmpFile\，文件名 exeName_pid_timestamp_x86|x64.dmp
// dump 类型枚举值使用 0x1126
// ---------------------------------------------------------------------------
static std::wstring WriteCrashDump(EXCEPTION_POINTERS* info)
{
	// 1. 定位 exe 目录与 exe 名（去扩展名）
	wchar_t exePath[MAX_PATH] = {};
	GetModuleFileNameW(nullptr, exePath, MAX_PATH);
	std::wstring exeDir(exePath);
	const size_t slash = exeDir.find_last_of(L"\\/");
	std::wstring exeName = (slash == std::wstring::npos) ? exeDir : exeDir.substr(slash + 1);
	if (slash != std::wstring::npos)
	{
		exeDir.resize(slash);
	}
	const size_t dot = exeName.find_last_of(L'.');
	if (dot != std::wstring::npos)
	{
		exeName.resize(dot);
	}

	// 2. 确保 DmpFile 目录存在
	const std::wstring dumpDir = exeDir + L"\\DmpFile";
	if (!CreateDirectoryW(dumpDir.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
	{
		return L"";
	}

	// 3. 组装文件名: exeName_pid_timestamp_x86|x64.dmp
#ifdef _WIN64
	const wchar_t* arch = L"x64";
#else
	const wchar_t* arch = L"x86";
#endif
	const std::wstring dumpPath = dumpDir + L"\\" + exeName +
		L"_" + std::to_wstring(GetCurrentProcessId()) +
		L"_" + std::to_wstring(static_cast<unsigned long long>(time(nullptr))) +
		L"_" + arch + L".dmp";

	// 4. MiniDumpWriteDump
	HANDLE hFile = CreateFileW(dumpPath.c_str(), GENERIC_WRITE, 0, nullptr,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return L"";
	}

	MINIDUMP_EXCEPTION_INFORMATION mei{};
	mei.ThreadId = GetCurrentThreadId();
	mei.ExceptionPointers = info;   // 本进程内存，ClientPointers = FALSE
	mei.ClientPointers = FALSE;

	const MINIDUMP_TYPE dumpType = static_cast<MINIDUMP_TYPE>(0x1126);
	const BOOL ok = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
		hFile, dumpType, &mei, nullptr, nullptr);
	CloseHandle(hFile);
	return ok ? dumpPath : L"";
}

// ---------------------------------------------------------------------------
// UEH: 异常最终落到这里（WER/崩溃采集的统一入口）
// ---------------------------------------------------------------------------
static LONG WINAPI MyUnhandledExceptionFilter(EXCEPTION_POINTERS* info)
{
	const EXCEPTION_RECORD* rec = info->ExceptionRecord;
	fprintf(stderr, "\n[UEH] UnhandledExceptionFilter entered (tid=%lu)\n", GetCurrentThreadId());
	fprintf(stderr, "[UEH]   code   = 0x%08X\n", rec->ExceptionCode);
	fprintf(stderr, "[UEH]   addr   = %p\n", rec->ExceptionAddress);
	fprintf(stderr, "[UEH]   flags  = 0x%08X\n", rec->ExceptionFlags);
	if (rec->ExceptionCode == kTerminateExceptionCode)
	{
		fprintf(stderr, "[UEH]   origin = hooked terminate (noexcept violation / uncaught exception)\n");
		fprintf(stderr, "[UEH]   reason = %s\n", g_lastReason);
	}

	// 生成 minidump 到 exe 目录\DmpFile\（类型 0x1126）
	const std::wstring dumpPath = WriteCrashDump(info);
	if (!dumpPath.empty())
	{
		fprintf(stderr, "[UEH]   dump written: %ls\n", dumpPath.c_str());
	}
	else
	{
		fprintf(stderr, "[UEH]   dump FAILED (error=%lu)\n", GetLastError());
	}
	fprintf(stderr, "[UEH]   -> report to crash server done\n");
	// 真实崩溃采集场景通常返回 EXCEPTION_CONTINUE_SEARCH，把控制权交给 WER
	// 生成崩溃报告/dump。此处为了演示直接结束进程。
	return EXCEPTION_EXECUTE_HANDLER;
}

// ---------------------------------------------------------------------------
// 安装 Detour
// ---------------------------------------------------------------------------
static bool AttachOne(PVOID* ppTarget, PVOID pDetour, const char* name)
{
	const LONG err = DetourAttach(ppTarget, pDetour);
	if (err != NO_ERROR)
	{
		fprintf(stderr, "[hook] DetourAttach(%s) failed: %ld\n", name, err);
		return false;
	}
	return true;
}

static bool InstallTerminateHooks()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	bool ok = true;
	ok &= AttachOne(reinterpret_cast<PVOID*>(&g_realTerminate),
		reinterpret_cast<PVOID>(&HookedTerminate), "std::terminate");
	ok &= AttachOne(reinterpret_cast<PVOID*>(&g_realStdTerminate),
		reinterpret_cast<PVOID>(&HookedTerminate), "__std_terminate");

	if (!ok || DetourTransactionCommit() != NO_ERROR)
	{
		DetourTransactionAbort();
		return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
// Demo
// ---------------------------------------------------------------------------
int main(int argc, char** argv)
{
	// UEH 路径会直接终止进程，stdout 改为无缓冲，保证演示日志不丢失
	setvbuf(stdout, nullptr, _IONBF, 0);
	SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
	printf("[main] UEH installed\n");

	if (!InstallTerminateHooks())
	{
		fprintf(stderr, "[main] failed to install terminate hooks\n");
		return 1;
	}
	printf("[main] hooked: std::terminate @%p, __std_terminate @%p\n",
		(void*)g_realTerminate, (void*)g_realStdTerminate);

	const int mode = (argc > 1) ? atoi(argv[1]) : 0;
	switch (mode)
	{
	case 1: // 主线程 noexcept 违例
		printf("\n[main] case 1: noexcept violation on main thread\n");
		[]() noexcept
			{
				throw std::runtime_error("noexcept violation on main thread");
			}();
		break;
	case 2: // 工作线程未捕获异常 -> std::terminate
		printf("\n[main] case 2: uncaught exception inside std::thread\n");
		{
			std::thread t([]
				{
					throw std::runtime_error("uncaught exception inside std::thread");
				});
			t.join();
		}
		break;
	case 3: // 显式 std::terminate()
		printf("\n[main] case 3: explicit std::terminate()\n");
		std::terminate();
		break;
	case 4:
	{
		int* p = nullptr;
		*p = 100;
		printf("%d", *p);
	}
	case 5:
	{
		std::jthread t([]()
			{
				int* p = nullptr;
				*p = 100;
				printf("%d", *p);
			});
		//t.join();
	}
	case 6:
	{
		OutputDebugStringA("[ConApp_UEH_demo] test log 1");//RaiseException 打印调试信息 不会引起异常到 UEH
		break;
	}
	case 7:
	{
		std::jthread t([]()
			{
				OutputDebugStringA("[ConApp_UEH_demo] test log 2");//RaiseException  打印调试信息 不会引起异常到 UEH
			});
		break;
	}
	default:
		printf("usage: terminate-hook.exe <1|2|3>\n"
			"  1 = noexcept violation on main thread\n"
			"  2 = uncaught exception inside std::thread\n"
			"  3 = explicit std::terminate()\n");
		break;
	}
	printf("[main] returned normally (demo modes should never reach here)\n");
	return 0;
}
