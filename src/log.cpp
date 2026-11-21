#include "log.h"

#include <cstdarg>
#include <cstdio>

namespace crashdiag
{
namespace
{
    IVEngineServer* g_engine = nullptr;
}

void SetLogEngine(IVEngineServer* engine)
{
    g_engine = engine;
}

void Log(const char* fmt, ...)
{
    char line[1024];

    va_list args;
    va_start(args, fmt);
    int n = std::vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    if (n < 0) return;

#if !defined(CRASHDIAG_STUB_SDK)
    // IVEngineServer::LogPrint takes a plain string and routes it to the
    // server console, so no tier0 dependency is involved.
    if (g_engine)
    {
        g_engine->LogPrint(line);
        return;
    }
#endif

    std::fputs(line, stderr);
    std::fflush(stderr);
}

} // namespace crashdiag
