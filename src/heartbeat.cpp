#include "heartbeat.h"
#include "crash_report.h"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>

#if defined(_WIN32)
  #include <windows.h>
  #include <psapi.h>
#endif

namespace crashdiag
{

float CurrentWorkingSetMB()
{
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS pmc;
    std::memset(&pmc, 0, sizeof(pmc));
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return (float)pmc.WorkingSetSize / (1024.0f * 1024.0f);
    return 0.0f;
#else
    return 0.0f;
#endif
}

void WriteHeartbeat(const CrashDiagSnapshot& snap)
{
    std::time_t now = std::time(nullptr);
    std::tm* local = std::localtime(&now);

    char stamp[32];
    std::snprintf(stamp, sizeof(stamp), "%04d-%02d-%02d %02d:%02d:%02d",
        local->tm_year + 1900, local->tm_mon + 1, local->tm_mday,
        local->tm_hour, local->tm_min, local->tm_sec);

    std::string path = std::string(LogDir()) + "\\heartbeat.log";
    FILE* f = std::fopen(path.c_str(), "a");
    if (!f) return;

    std::fprintf(f,
        "%s | ticks=%-9lu map=%-24s players=%-3d lvInit=%-5lu lvShut=%-5lu "
        "clActive=%-5lu clDisc=%-5lu esCmds=%-6lu avgFrameMs=%.3f wsMB=%.1f\n",
        stamp,
        (unsigned long)snap.ticks,
        snap.mapName,
        snap.players,
        (unsigned long)snap.counters.lvInit,
        (unsigned long)snap.counters.lvShut,
        (unsigned long)snap.counters.clActive,
        (unsigned long)snap.counters.clDisc,
        (unsigned long)snap.counters.esCmds,
        snap.avgFrameMs,
        snap.wsMB);

    std::fclose(f);
}

} // namespace crashdiag
