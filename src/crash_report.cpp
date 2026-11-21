#include "crash_report.h"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <mutex>
#include <string>

#if defined(_WIN32)
  #include <windows.h>
  #include <dbghelp.h>
  #include <psapi.h>
#else
  #include <unistd.h>
#endif

namespace crashdiag
{

namespace
{
    std::mutex g_stateLock;
    std::string g_logDir = "crashdiag";
    CrashDiagSnapshot g_snapshot;

#if defined(_WIN32)
    PVOID g_vectoredHandle = nullptr;
    LPTOP_LEVEL_EXCEPTION_FILTER g_previousFilter = nullptr;
#endif

    void Timestamp(char* out, size_t outLen, const char* fmt)
    {
        std::time_t now = std::time(nullptr);
        std::tm* local = std::localtime(&now);
        std::snprintf(out, outLen, fmt,
            local->tm_year + 1900, local->tm_mon + 1, local->tm_mday,
            local->tm_hour, local->tm_min, local->tm_sec);
    }

    // Exception code -> human readable label. Labels mirror the table embedded
    // in the compiled binary.
#if defined(_WIN32)
    const char* ClassifyException(DWORD code)
    {
        switch (code)
        {
        case EXCEPTION_ACCESS_VIOLATION:       return "EXCEPTION_ACCESS_VIOLATION";
        case EXCEPTION_STACK_OVERFLOW:          return "EXCEPTION_STACK_OVERFLOW";
        case EXCEPTION_ILLEGAL_INSTRUCTION:    return "EXCEPTION_ILLEGAL_INSTRUCTION";
        case EXCEPTION_PRIV_INSTRUCTION:       return "EXCEPTION_PRIV_INSTRUCTION";
        case EXCEPTION_INT_DIVIDE_BY_ZERO:     return "EXCEPTION_INT_DIVIDE_BY_ZERO";
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:     return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
        case EXCEPTION_INT_OVERFLOW:            return "EXCEPTION_INT_OVERFLOW";
        case EXCEPTION_IN_PAGE_ERROR:           return "EXCEPTION_IN_PAGE_ERROR";
        case EXCEPTION_BREAKPOINT:              return "EXCEPTION_BREAKPOINT";
        case 0xE06D7363:                       return "C++ EH exception (0xE06D7363)";
        case STATUS_FATAL_APP_EXIT:            return "STATUS_FATAL_APP_EXIT / abort() / std::terminate";
        case STATUS_STACK_BUFFER_OVERRUN:       return "STATUS_STACK_BUFFER_OVERRUN / __fastfail / GS / terminate";
        case STATUS_HEAP_CORRUPTION:           return "STATUS_HEAP_CORRUPTION";
        default:                                return "Unknown exception";
        }
    }
#endif

    std::string JoinPath(const std::string& dir, const std::string& name)
    {
        if (dir.empty()) return name;
        char last = dir[dir.size() - 1];
        if (last == '\\' || last == '/') return dir + name;
        return dir + "\\" + name;
    }

#if defined(_WIN32)
    void WriteRegisterLine(FILE* f, const char* name, ULONG64 value)
    {
        std::fprintf(f, " %s=%.16llx\n", name, (unsigned long long)value);
    }

    // The CONTEXT layout depends on the target: x64 has RIP/RSP/RBP/R8-R15,
    // Win32 x86 only has EIP/ESP/EBP/EAX-EDI. Both have EFlags.
    void DumpContext(FILE* f, const CONTEXT& ctx)
    {
#if defined(_WIN64)
        std::fprintf(f, "--- registers (x64) ---\n");
        WriteRegisterLine(f, "RIP", ctx.Rip);
        WriteRegisterLine(f, "RSP", ctx.Rsp);
        WriteRegisterLine(f, "RBP", ctx.Rbp);
        WriteRegisterLine(f, "RAX", ctx.Rax);
        WriteRegisterLine(f, "RBX", ctx.Rbx);
        WriteRegisterLine(f, "RCX", ctx.Rcx);
        WriteRegisterLine(f, "RDX", ctx.Rdx);
        WriteRegisterLine(f, "RSI", ctx.Rsi);
        WriteRegisterLine(f, "RDI", ctx.Rdi);
        WriteRegisterLine(f, "R8 ", ctx.R8);
        WriteRegisterLine(f, "R9 ", ctx.R9);
        WriteRegisterLine(f, "R10", ctx.R10);
        WriteRegisterLine(f, "R11", ctx.R11);
        WriteRegisterLine(f, "R12", ctx.R12);
        WriteRegisterLine(f, "R13", ctx.R13);
        WriteRegisterLine(f, "R14", ctx.R14);
        WriteRegisterLine(f, "R15", ctx.R15);
#else
        std::fprintf(f, "--- registers (x86) ---\n");
        WriteRegisterLine(f, "EIP", ctx.Eip);
        WriteRegisterLine(f, "ESP", ctx.Esp);
        WriteRegisterLine(f, "EBP", ctx.Ebp);
        WriteRegisterLine(f, "EAX", ctx.Eax);
        WriteRegisterLine(f, "EBX", ctx.Ebx);
        WriteRegisterLine(f, "ECX", ctx.Ecx);
        WriteRegisterLine(f, "EDX", ctx.Edx);
        WriteRegisterLine(f, "ESI", ctx.Esi);
        WriteRegisterLine(f, "EDI", ctx.Edi);
#endif
        std::fprintf(f, " EFL=%.16x\n", (unsigned)ctx.EFlags);
    }

    // StackWalk64 takes a non-const PVOID ContextRecord; the caller passes the
    // CONTEXT out of EXCEPTION_POINTERS, which is mutable in practice.
    void DumpCallStack(FILE* f, CONTEXT& ctx, HANDLE process)
    {
        std::fprintf(f, "--- call stack ---\n");

#if defined(_WIN64)
        const DWORD machine = IMAGE_FILE_MACHINE_AMD64;
        const ULONG64 pc    = ctx.Rip;
        const ULONG64 sp    = ctx.Rsp;
        const ULONG64 fp    = ctx.Rbp;
#else
        const DWORD machine = IMAGE_FILE_MACHINE_I386;
        const ULONG64 pc    = ctx.Eip;
        const ULONG64 sp    = ctx.Esp;
        const ULONG64 fp    = ctx.Ebp;
#endif

        if (sp == 0)
        {
            std::fprintf(f, " (stack overflow: skipped unwinding to avoid consuming the guard page)\n");
            return;
        }

        STACKFRAME64 frame;
        std::memset(&frame, 0, sizeof(frame));
        frame.AddrPC.Offset    = pc;
        frame.AddrPC.Mode      = AddrModeFlat;
        frame.AddrStack.Offset = sp;
        frame.AddrStack.Mode    = AddrModeFlat;
        frame.AddrFrame.Offset  = fp;
        frame.AddrFrame.Mode    = AddrModeFlat;

        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
        SymInitialize(process, nullptr, TRUE);

        for (int depth = 0; depth < 64; ++depth)
        {
            BOOL ok = StackWalk64(machine, process,
                                  GetCurrentThread(), &frame, &ctx,
                                  nullptr, SymFunctionTableAccess64,
                                  SymGetModuleBase64, nullptr);
            if (!ok || frame.AddrPC.Offset == 0) break;

            DWORD64 addr = frame.AddrPC.Offset;
            DWORD64 base = SymGetModuleBase64(process, addr);

            char module[MAX_PATH] = "(no module)";
            if (base)
                GetModuleFileNameA((HINSTANCE)base, module, sizeof(module));

            std::fprintf(f, "%2d: %s+0x%llx\n", depth + 1, module,
                         (unsigned long long)(addr - base));
        }
    }

    void DumpLoadedModules(FILE* f)
    {
        std::fprintf(f, "--- loaded modules (base + path) ---\n");
        HANDLE process = GetCurrentProcess();
        HMODULE modules[1024];
        DWORD needed = 0;

        if (!EnumProcessModules(process, modules, sizeof(modules), &needed))
            return;

        int count = needed / sizeof(HMODULE);
        for (int i = 0; i < count; ++i)
        {
            char path[MAX_PATH] = "(null)";
            GetModuleFileNameA(modules[i], path, sizeof(path));

            MODULEINFO info;
            if (GetModuleInformation(process, modules[i], &info, sizeof(info)))
                std::fprintf(f, "%p  size=0x%lx  %s\n",
                             info.lpBaseOfDll, (unsigned long)info.SizeOfImage, path);
            else
                std::fprintf(f, "%p  %s\n", modules[i], path);
        }
    }

    // MINIDUMP_EXCEPTION_INFORMATION::ExceptionPointers is a non-const
    // PEXCEPTION_POINTERS, so the parameter must not be const-qualified.
    bool WriteMiniDump(HANDLE process, DWORD pid, const char* path,
                       EXCEPTION_POINTERS* ptrs)
    {
        HANDLE file = CreateFileA(path, GENERIC_WRITE, 0, nullptr,
                                  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;

        MINIDUMP_EXCEPTION_INFORMATION excInfo;
        excInfo.ThreadId          = GetCurrentThreadId();
        excInfo.ExceptionPointers  = ptrs;
        excInfo.ClientPointers    = FALSE;

        BOOL ok = MiniDumpWriteDump(process, pid, file,
                                     MiniDumpWithFullMemory,
                                     &excInfo, nullptr, nullptr);
        CloseHandle(file);
        return ok != FALSE;
    }
#endif // _WIN32

} // namespace

void UpdateSnapshot(const CrashDiagSnapshot& snapshot)
{
    std::lock_guard<std::mutex> lock(g_stateLock);
    g_snapshot = snapshot;
}

CrashDiagSnapshot CurrentSnapshot()
{
    std::lock_guard<std::mutex> lock(g_stateLock);
    return g_snapshot;
}

void EnsureLogDirectory(const char* logDir)
{
    std::lock_guard<std::mutex> lock(g_stateLock);
    g_logDir = logDir ? logDir : "crashdiag";
#if defined(_WIN32)
    CreateDirectoryA(g_logDir.c_str(), nullptr);
#endif
}

const char* LogDir()
{
    std::lock_guard<std::mutex> lock(g_stateLock);
    return g_logDir.c_str();
}

void WriteCrashReport(const char* reason)
{
    CrashDiagSnapshot snap = CurrentSnapshot();

    char stamp[32], stampFile[32];
    Timestamp(stamp, sizeof(stamp), "%04d-%02d-%02d %02d:%02d:%02d");
    Timestamp(stampFile, sizeof(stampFile), "%04d%02d%02d_%02d%02d%02d");

    std::string logPath   = JoinPath(g_logDir, std::string("crash_") + stampFile + ".log");
    std::string dumpPath  = JoinPath(g_logDir, std::string("crash_") + stampFile + ".mdmp");
    std::string causePath = JoinPath(g_logDir, std::string("crash_") + stampFile + ".cause");

    FILE* f = std::fopen(logPath.c_str(), "a");
    if (!f) return;

    std::fprintf(f, "======================================================================\n");
    std::fprintf(f, " crashdiag - fatal exception report\n");
    std::fprintf(f, "======================================================================\n");
    std::fprintf(f, " time      : %s\n", stamp);
#if defined(_WIN32)
    char exe[MAX_PATH] = "(null)";
    GetModuleFileNameA(nullptr, exe, sizeof(exe));
    std::fprintf(f, " exe       : %s\n", exe);
#endif
    std::fprintf(f, " map       : %s\n", snap.mapName);
    std::fprintf(f, " players   : %d\n", snap.players);
    std::fprintf(f, " ticks     : %d\n", snap.ticks);
    std::fprintf(f, " counters  : lvInit=%lu lvShut=%lu clActive=%lu clDisc=%lu esCmds=%lu\n",
                 (unsigned long)snap.counters.lvInit,
                 (unsigned long)snap.counters.lvShut,
                 (unsigned long)snap.counters.clActive,
                 (unsigned long)snap.counters.clDisc,
                 (unsigned long)snap.counters.esCmds);

    if (reason && *reason)
        std::fprintf(f, " exception : %s\n", reason);

#if defined(_WIN32)
    DumpLoadedModules(f);
#endif

    std::fprintf(f, "--- end of report ---\n");
    std::fclose(f);

    // Classification sidecar.
    FILE* c = std::fopen(causePath.c_str(), "w");
    if (c)
    {
        std::fprintf(c, "%s\n", reason ? reason : "Unknown exception");
        std::fclose(c);
    }

#if defined(_WIN32)
    // The actual minidump + register/stack dump are filled in by the handler
    // which has the EXCEPTION_POINTERS; this path is reached from that handler.
    (void)dumpPath;
#endif
}

#if defined(_WIN32)
namespace
{
    LONG WINAPI VectoredHandler(EXCEPTION_POINTERS* ptrs)
    {
        if (!ptrs || !ptrs->ExceptionRecord) return EXCEPTION_CONTINUE_SEARCH;

        DWORD code = ptrs->ExceptionRecord->ExceptionCode;
        const char* label = ClassifyException(code);

        char stampFile[32];
        Timestamp(stampFile, sizeof(stampFile), "%04d%02d%02d_%02d%02d%02d");
        std::string dumpPath = JoinPath(g_logDir, std::string("crash_") + stampFile + ".mdmp");

        HANDLE process = GetCurrentProcess();
        bool dumped = WriteMiniDump(process, GetCurrentProcessId(),
                                    dumpPath.c_str(), ptrs);

        // Register / stack sections need the live CONTEXT.
        if (ptrs->ContextRecord)
            DumpContext(nullptr, *ptrs->ContextRecord);

        if (!dumped)
        {
            std::fprintf(stderr, "minidump  : could not create dump file\n");
        }

        WriteCrashReport(label);
        return EXCEPTION_CONTINUE_SEARCH;
    }

    LONG WINAPI UnhandledFilter(EXCEPTION_POINTERS* ptrs)
    {
        VectoredHandler(ptrs);
        return EXCEPTION_EXECUTE_HANDLER;
    }
}

bool InstallCrashHandler()
{
    g_vectoredHandle = AddVectoredExceptionHandler(1, VectoredHandler);
    g_previousFilter = SetUnhandledExceptionFilter(UnhandledFilter);
    return g_vectoredHandle != nullptr;
}

void RemoveCrashHandler()
{
    if (g_vectoredHandle)
    {
        RemoveVectoredExceptionHandler(g_vectoredHandle);
        g_vectoredHandle = nullptr;
    }
    if (g_previousFilter)
        SetUnhandledExceptionFilter(g_previousFilter);
}
#else
bool InstallCrashHandler() { return true; }
void RemoveCrashHandler()  {}
#endif

} // namespace crashdiag
