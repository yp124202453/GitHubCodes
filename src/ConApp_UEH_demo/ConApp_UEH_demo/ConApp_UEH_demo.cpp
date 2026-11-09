// terminate-hook.cpp
//
// 用 Microsoft Detours 内联 hook std::terminate / __std_terminate，
// 把 C++ 层的 terminate 路径（noexcept 违例、std::thread 未捕获异常、
// 显式 std::terminate()）改写成 RaiseException 抛出 SEH 异常，
// 使其流经 UEH (SetUnhandledExceptionFilter / WER)，
// 便于崩溃上报系统（WER、LocalDumps、第三方 crash handler）统一采集。
//
// 编译: vcpkg 原目录依赖 detours + fmt（C++ 格式化统一用 fmtlib）+ MSVC。

#include <windows.h>
#include <dbghelp.h>
#include <detours.h>

#include <exception>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>      // errno_t
#include <ctime>
#include <iterator>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <stdexcept>
#include <thread>

#include <fmt/format.h>
#include <fmt/xchar.h>   // fmt 12 起宽字符格式化（L"..." / std::wstring）单独在此头文件

// ---------------------------------------------------------------------------
// msvcp140.dll 的 ABI 入口：MSVC 编译器在 noexcept 违例时直接调用它，
// 不保证经过 std::terminate()。因此两个入口都要 hook。
// __std_terminate 由 <exception>/<yvals.h> 标准头声明，签名以头文件为准。
// ---------------------------------------------------------------------------

// 自定义异常码: 0xE000xxxx = customer + severity error，标记为不可继续
static constexpr DWORD kTerminateExceptionCode = 0xE0005443; // "TC" = Terminate->Crash

using TerminateFn = void (*)();
static TerminateFn g_realTerminate = std::terminate;

// 别手写调用约定：x86 下 __std_terminate 是 __cdecl，x64 下是 __stdcall，
// 且 noexcept 规范也随版本变化，直接以头文件里的声明为准。
using StdTerminateFn = decltype(&__std_terminate);
static StdTerminateFn g_realStdTerminate = __std_terminate;

// 供 UEH 读取的“终止原因”
static char g_lastReason[512] = {};

// 显式 std::terminate() 的退出码
static constexpr UINT kExplicitTerminateExitCode = 1;

// 参数解析允许的最大长度：超长输入直接按非法输入处理
static constexpr size_t kMaxArgLen = 64;

// ---------------------------------------------------------------------------
// 字符串处理分工（全局规则）：
//   - 格式化 / 拼接统一用 fmtlib；format_to_n 写固定缓冲、零分配，
//     适合 UEH / terminate 这类崩溃上下文，并检查返回的 size 判断是否截断
//   - 纯拷贝 / 长度 / 输入解析仍用 _s 系列，每次调用都判断返回值
// ---------------------------------------------------------------------------
static void SetReasonLiteral(const char* text) {
    const errno_t err = strcpy_s(g_lastReason, text);
    if (err != 0) {
        g_lastReason[0] = '\0';
    }
}

static void SetReasonFormatted(const char* prefix, const char* detail) {
    // format_to_n 最多写 sizeof-1 个字符，绝不越界；result.size 是「未截断时
    // 会写入的总字符数」，用它判断是否发生截断。
    const auto result = fmt::format_to_n(g_lastReason, sizeof(g_lastReason) - 1,
                                         "{}{}", prefix, detail);
    *result.out = '\0';
    if (result.size >= sizeof(g_lastReason)) {
        // 截断：尾部改成省略号，明确标记信息不完整；memcpy_s 同样检查 errno_t
        constexpr char kEllipsis[] = "...";
        constexpr size_t kLen = sizeof(kEllipsis) - 1;
        char* tail = g_lastReason + sizeof(g_lastReason) - 1 - kLen;
        const errno_t err = memcpy_s(tail, kLen, kEllipsis, kLen);
        if (err != 0) {
            g_lastReason[0] = '\0';
        }
    }
}

// ---------------------------------------------------------------------------
// UTF-8 <-> UTF-16 (wchar_t) 双向转换，完整支持中文、日文等 BMP 字符、
// 以及 emoji 这类需要代理对 (surrogate pair) 的增补平面字符。
//
// 设计要点：
//   - 显式传长度而不是 -1：字符串中间的 '\0' / L'\0' 不会被当作结尾截断
//   - 严格模式 MB_ERR_INVALID_CHARS / WC_ERR_INVALID_CHARS：非法字节、
//     被截断的多字节序列、孤立代理项一律失败，绝不静默替换成 U+FFFD
//   - 返回 std::optional：空输入是「成功的空串」，nullopt 才代表失败；
//     调用方必须判断，不允许把失败当成空串吞掉
//   - 两次 Win32 调用都检查返回值；长度超过 INT_MAX 直接判失败，避免窄化
// ---------------------------------------------------------------------------
static bool FitsInInt(size_t count) {
    // 括号包住 max()，规避 windows.h 的类函数宏 max(a,b)
    return count <= static_cast<size_t>((std::numeric_limits<int>::max)());
}

static std::optional<std::wstring> Utf8ToWide(std::string_view utf8) {
    if (utf8.empty()) {
        return std::wstring{};
    }
    if (!FitsInInt(utf8.size())) {
        return std::nullopt;
    }
    const int inLen = static_cast<int>(utf8.size());
    const int needed = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                           utf8.data(), inLen, nullptr, 0);
    if (needed <= 0) {
        return std::nullopt;
    }
    std::wstring out(static_cast<size_t>(needed), L'\0');
    const int written = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                            utf8.data(), inLen, out.data(), needed);
    if (written != needed) {
        return std::nullopt;
    }
    return out;
}

static std::optional<std::string> WideToUtf8(std::wstring_view wide) {
    if (wide.empty()) {
        return std::string{};
    }
    if (!FitsInInt(wide.size())) {
        return std::nullopt;
    }
    const int inLen = static_cast<int>(wide.size());
    const int needed = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                           wide.data(), inLen,
                                           nullptr, 0, nullptr, nullptr);
    if (needed <= 0) {
        return std::nullopt;
    }
    std::string out(static_cast<size_t>(needed), '\0');
    const int written = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                            wide.data(), inLen,
                                            out.data(), needed, nullptr, nullptr);
    if (written != needed) {
        return std::nullopt;
    }
    return out;
}

// ---------------------------------------------------------------------------
// terminate 被拦截时，把仍在飞行中的异常对象取出来（rethrow 到 catch），
// 记录 what() 信息，然后抛 SEH 异常走 UEH。
// ---------------------------------------------------------------------------
static void CaptureCurrentException() {
    try {
        if (std::current_exception()) {
            std::rethrow_exception(std::current_exception());
        }
    } catch (const std::exception& e) {
        SetReasonFormatted("std::exception: ", e.what());
        return;
    } catch (...) {
        SetReasonLiteral("non-standard exception object");
        return;
    }
    SetReasonLiteral("terminate called without active exception");
}

// ---------------------------------------------------------------------------
// Detour: 替代原 terminate —— 不再走 abort()，而是 RaiseException
// ---------------------------------------------------------------------------
static void HookedTerminate() noexcept {
    CaptureCurrentException();

    // 无活动异常 => 用户代码显式调用 std::terminate()：
    // 不生成 dump、不走 UEH，直接退出进程
    if (std::strcmp(g_lastReason, "terminate called without active exception") == 0) {
        fmt::print(stderr, "[hook] explicit std::terminate() (no active exception) -> no dump, exiting process directly\n");
        fflush(stderr);
        ExitProcess(kExplicitTerminateExitCode);
    }

    fmt::print(stderr, "[hook] terminate intercepted! reason: {}\n", g_lastReason);
    fmt::print(stderr, "[hook] raising SEH exception 0x{:08X} -> dispatching to UEH...\n",
               kTerminateExceptionCode);
    fflush(stderr);

    // NONCONTINUABLE + 无 SEH handler => 异常分发交由 UEH / WER 处理，
    // 本函数永不返回（进程在此路径上结束）。
    RaiseException(kTerminateExceptionCode, EXCEPTION_NONCONTINUABLE, 0, nullptr);

    // 理论上不可达；防御性兜底
    for (;;) {
        Sleep(1000);
    }
}

// ---------------------------------------------------------------------------
// MiniDump 生成：写到 exe 目录\DmpFile\，文件名 exeName_pid_timestamp_x86|x64.dmp
// dump 类型枚举值使用 0x1126
// ---------------------------------------------------------------------------
static std::wstring WriteCrashDump(EXCEPTION_POINTERS* info) {
    // 1. 定位 exe 目录与 exe 名（去扩展名）。
    //    动态扩展缓冲，避免长路径被 MAX_PATH 截断（UEH 回调中同样安全）
    std::wstring exePath(MAX_PATH, L'\0');
    for (;;) {
        const DWORD n = GetModuleFileNameW(nullptr, exePath.data(), static_cast<DWORD>(exePath.size()));
        if (n == 0) {
            return L"";
        }
        if (n < exePath.size()) {
            exePath.resize(n);
            break;
        }
        exePath.resize(exePath.size() * 2);
    }
    std::wstring exeDir(exePath);
    const size_t slash = exeDir.find_last_of(L"\\/");
    std::wstring exeName = (slash == std::wstring::npos) ? exeDir : exeDir.substr(slash + 1);
    if (slash != std::wstring::npos) {
        exeDir.resize(slash);
    }
    const size_t dot = exeName.find_last_of(L'.');
    if (dot != std::wstring::npos) {
        exeName.resize(dot);
    }

    // 2. 确保 DmpFile 目录存在
    const std::wstring dumpDir = fmt::format(L"{}\\DmpFile", exeDir);
    if (!CreateDirectoryW(dumpDir.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) {
        return L"";
    }

    // 3. 组装文件名: exeName_pid_timestamp_x86|x64.dmp（fmtlib 宽字符串格式化）
#ifdef _WIN64
    const wchar_t* arch = L"x64";
#else
    const wchar_t* arch = L"x86";
#endif
    const std::wstring dumpPath = fmt::format(L"{}\\{}_{}_{}_{}.dmp",
                                              dumpDir, exeName,
                                              GetCurrentProcessId(),
                                              static_cast<unsigned long long>(time(nullptr)),
                                              arch);

    // 4. MiniDumpWriteDump
    HANDLE hFile = CreateFileW(dumpPath.c_str(), GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
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
static LONG WINAPI MyUnhandledExceptionFilter(EXCEPTION_POINTERS* info) {
    const EXCEPTION_RECORD* rec = info->ExceptionRecord;
    fmt::print(stderr, "\n[UEH] UnhandledExceptionFilter entered (tid={})\n", GetCurrentThreadId());
    fmt::print(stderr, "[UEH]   code   = 0x{:08X}\n", rec->ExceptionCode);
    fmt::print(stderr, "[UEH]   addr   = {}\n", rec->ExceptionAddress);
    fmt::print(stderr, "[UEH]   flags  = 0x{:08X}\n", rec->ExceptionFlags);
    if (rec->ExceptionCode == kTerminateExceptionCode) {
        fmt::print(stderr, "[UEH]   origin = hooked terminate (noexcept violation / uncaught exception)\n");
        fmt::print(stderr, "[UEH]   reason = {}\n", g_lastReason);
    }

    // 生成 minidump 到 exe 目录\DmpFile\（类型 0x1126）
    const std::wstring dumpPath = WriteCrashDump(info);
    if (!dumpPath.empty()) {
        const auto pathUtf8 = WideToUtf8(dumpPath);
        if (pathUtf8) {
            fmt::print(stderr, "[UEH]   dump written: {}\n", *pathUtf8);
        } else {
            fmt::print(stderr, "[UEH]   dump written: <path conversion failed, error={}>\n",
                       GetLastError());
        }
    } else {
        fmt::print(stderr, "[UEH]   dump FAILED (error={})\n", GetLastError());
    }
    fmt::print(stderr, "[UEH]   -> report to crash server done\n");
    // 真实崩溃采集场景通常返回 EXCEPTION_CONTINUE_SEARCH，把控制权交给 WER
    // 生成崩溃报告/dump。此处为了演示直接结束进程。
    return EXCEPTION_EXECUTE_HANDLER;
}

// ---------------------------------------------------------------------------
// 安装 Detour
// ---------------------------------------------------------------------------
static bool AttachOne(PVOID* ppTarget, PVOID pDetour, const char* name) {
    const LONG err = DetourAttach(ppTarget, pDetour);
    if (err != NO_ERROR) {
        fmt::print(stderr, "[hook] DetourAttach({}) failed: {}\n", name, err);
        return false;
    }
    return true;
}

static bool InstallTerminateHooks() {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    bool ok = true;
    ok &= AttachOne(reinterpret_cast<PVOID*>(&g_realTerminate),
                    reinterpret_cast<PVOID>(&HookedTerminate), "std::terminate");
    ok &= AttachOne(reinterpret_cast<PVOID*>(&g_realStdTerminate),
                    reinterpret_cast<PVOID>(&HookedTerminate), "__std_terminate");

    if (!ok || DetourTransactionCommit() != NO_ERROR) {
        DetourTransactionAbort();
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// mode 4：UTF-8 / wide 转换自测，每个方向都覆盖
// 正常路径 / 空输入与边界 / 失败路径，源码保留供手动复跑。
// ---------------------------------------------------------------------------
// 只在本函数作用域内把控制台切到 UTF-8，退出时还原，不污染调用者的终端。
class ConsoleUtf8Scope {
public:
    ConsoleUtf8Scope() : previous_(GetConsoleOutputCP()) {
        // 输出被重定向时可能没有控制台，设置失败可忽略（文件写的是原始 UTF-8 字节）
        (void)SetConsoleOutputCP(CP_UTF8);
    }
    ~ConsoleUtf8Scope() {
        if (previous_ != 0) {
            (void)SetConsoleOutputCP(previous_);
        }
    }
    ConsoleUtf8Scope(const ConsoleUtf8Scope&) = delete;
    ConsoleUtf8Scope& operator=(const ConsoleUtf8Scope&) = delete;

private:
    UINT previous_;
};

static bool ExpectEncoding(bool condition, const std::string& label) {
    const bool ok = condition;
    fmt::print("[enc] {:<5} {}\n", ok ? "PASS" : "FAIL", label);
    return ok;
}

static bool TestEncodingNormal() {
    struct Sample {
        std::string_view utf8;
        std::wstring_view wide;
        const char* name;
    };
    const Sample samples[] = {
        {"中文测试", L"中文测试", "chinese only"},
        {"崩溃转储 CrashDump 2026", L"崩溃转储 CrashDump 2026", "chinese + ascii"},
        {"中文—破折号", L"中文—破折号", "3-byte punctuation"},
        {"崩溃 🛠 已捕获", L"崩溃 🛠 已捕获", "emoji surrogate pair"},
    };
    bool ok = true;
    for (const auto& s : samples) {
        const auto wide = Utf8ToWide(s.utf8);
        const auto back = wide ? WideToUtf8(*wide) : std::optional<std::string>{};
        if (!ExpectEncoding(wide && std::wstring_view(*wide) == s.wide,
                            fmt::format("{}/utf8->wide", s.name))) {
            ok = false;
        }
        if (!ExpectEncoding(back && std::string_view(*back) == s.utf8,
                            fmt::format("{}/round-trip", s.name))) {
            ok = false;
        }
    }
    return ok;
}

static bool TestEncodingBoundary() {
    bool ok = true;

    // 空输入必须「成功返回空串」，不能被当成转换失败
    const auto emptyWide = Utf8ToWide(std::string_view{});
    if (!ExpectEncoding(emptyWide && emptyWide->empty(), "empty utf8 -> empty wide")) {
        ok = false;
    }
    const auto emptyUtf8 = WideToUtf8(std::wstring_view{});
    if (!ExpectEncoding(emptyUtf8 && emptyUtf8->empty(), "empty wide -> empty utf8")) {
        ok = false;
    }

    // 显式传长度：中间的 NUL 不截断
    std::string withNul = "中文";
    withNul.push_back('\0');
    withNul += "测试";
    std::wstring expectedWide = L"中文";
    expectedWide.push_back(L'\0');
    expectedWide += L"测试";
    const auto wide = Utf8ToWide(withNul);
    if (!ExpectEncoding(wide && *wide == expectedWide, "embedded NUL utf8->wide")) {
        ok = false;
    }
    const auto back = wide ? WideToUtf8(*wide) : std::optional<std::string>{};
    if (!ExpectEncoding(back && *back == withNul, "embedded NUL round-trip")) {
        ok = false;
    }

    // 长输入：验证缓冲长度计算与 int 上限内的正确性
    constexpr size_t kRepeat = 10000;
    std::string longUtf8;
    longUtf8.reserve(kRepeat * 3);
    for (size_t i = 0; i < kRepeat; ++i) {
        longUtf8 += "中";
    }
    const auto longWide = Utf8ToWide(longUtf8);
    if (!ExpectEncoding(longWide && longWide->size() == kRepeat, "long input (10000 chinese chars)")) {
        ok = false;
    }
    const auto longBack = longWide ? WideToUtf8(*longWide) : std::optional<std::string>{};
    if (!ExpectEncoding(longBack && *longBack == longUtf8, "long input round-trip")) {
        ok = false;
    }
    return ok;
}

static bool TestEncodingFailure() {
    bool ok = true;
    const std::string_view badUtf8[] = {
        std::string_view("\xFF\xFE", 2),         // 非法起始字节
        std::string_view("\xE4\xB8", 2),         // "中" 被截断，缺第三字节
        std::string_view("\xED\xA0\x80", 3),     // CESU-8 代理项编码
    };
    for (size_t i = 0; i < std::size(badUtf8); ++i) {
        if (!ExpectEncoding(!Utf8ToWide(badUtf8[i]).has_value(),
                            fmt::format("invalid utf8 #{} rejected", i))) {
            ok = false;
        }
    }
    const wchar_t loneHigh[] = {0xD800, L'A'};
    const wchar_t loneLow[] = {0xDC00, L'A'};
    const std::wstring_view badWide[] = {
        std::wstring_view(loneHigh, 1),
        std::wstring_view(loneLow, 1),
    };
    for (size_t i = 0; i < std::size(badWide); ++i) {
        if (!ExpectEncoding(!WideToUtf8(badWide[i]).has_value(),
                            fmt::format("lone surrogate #{} rejected", i))) {
            ok = false;
        }
    }
    return ok;
}

static int RunEncodingTests(const std::string& group) {
    // 控制台按 UTF-8 渲染，方便人工核对中文样例；退出作用域自动还原
    const ConsoleUtf8Scope consoleUtf8;

    bool ok = true;
    if (group == "all") {
        if (!TestEncodingNormal()) {
            ok = false;
        }
        if (!TestEncodingBoundary()) {
            ok = false;
        }
        if (!TestEncodingFailure()) {
            ok = false;
        }
    } else if (group == "normal") {
        ok = TestEncodingNormal();
    } else if (group == "boundary") {
        ok = TestEncodingBoundary();
    } else if (group == "failure") {
        ok = TestEncodingFailure();
    } else {
        fmt::print("usage: terminate-hook.exe 4 <all|normal|boundary|failure>\n");
        return 0;
    }

    const auto wide = Utf8ToWide("中文 → wide → UTF-8 往返正常");
    const auto back = wide ? WideToUtf8(*wide) : std::optional<std::string>{};
    fmt::print("[enc] sample: {}\n", back.value_or("<conversion failed>"));
    fmt::print("[enc] group={} result={}\n", group, ok ? "PASS" : "FAIL");
    return ok ? 0 : 2;
}

// ---------------------------------------------------------------------------
// Demo
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    // UEH 路径会直接终止进程，stdout 改为无缓冲，保证演示日志不丢失
    setvbuf(stdout, nullptr, _IONBF, 0);
    SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
    fmt::print("[main] UEH installed\n");

    if (!InstallTerminateHooks()) {
        fmt::print(stderr, "[main] failed to install terminate hooks\n");
        return 1;
    }
    fmt::print("[main] hooked: std::terminate @{}, __std_terminate @{}\n",
               (void*)g_realTerminate, (void*)g_realStdTerminate);

    // 用 _s 系列解析参数，避免 atoi 的越界/无检验行为。
    // strnlen_s 返回长度（空输入为 0，达到上限即超长）；_snscanf_s 返回成功
    // 赋值的字段数。两者都必须检查，任一不合格都回落到 usage。
    int mode = 0;
    if (argc > 1) {
        const size_t argLen = strnlen_s(argv[1], kMaxArgLen);
        if (argLen == 0 || argLen >= kMaxArgLen) {
            mode = 0;
        } else {
            const int parsed = _snscanf_s(argv[1], argLen, "%d", &mode);
            if (parsed != 1) {
                mode = 0;
            }
        }
    }
    // mode 4 的自测子组同样走 _s 长度检查：空/超长都落到 usage
    std::string encGroup = "all";
    if (mode == 4 && argc > 2) {
        const size_t groupLen = strnlen_s(argv[2], kMaxArgLen);
        if (groupLen == 0 || groupLen >= kMaxArgLen) {
            encGroup = "<invalid>";
        } else {
            encGroup.assign(argv[2], groupLen);
        }
    }

    switch (mode) {
    case 1: // 主线程 noexcept 违例
        fmt::print("\n[main] case 1: noexcept violation on main thread\n");
        []() noexcept { throw std::runtime_error("noexcept violation on main thread"); }();
        break;
    case 2: // 工作线程未捕获异常 -> std::terminate
        fmt::print("\n[main] case 2: uncaught exception inside std::thread\n");
        {
            std::thread t([] { throw std::runtime_error("uncaught exception inside std::thread"); });
            t.join();
        }
        break;
    case 3: // 显式 std::terminate()
        fmt::print("\n[main] case 3: explicit std::terminate()\n");
        std::terminate();
        break;
    case 4: { // UTF-8 <-> wide 转换自测（含中文）
        return RunEncodingTests(encGroup);
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
        fmt::print("usage: terminate-hook.exe <1|2|3|4>\n"
                   "  1 = noexcept violation on main thread\n"
                   "  2 = uncaught exception inside std::thread\n"
                   "  3 = explicit std::terminate()\n"
                   "  4 = UTF-8 <-> wide self-test [all|normal|boundary|failure]\n");
        break;
    }
    fmt::print("[main] returned normally (demo modes should never reach here)\n");
    return 0;
}
