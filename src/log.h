// Logging that does not depend on tier0's ConMsg.
//
// ConMsg lives in tier0.lib, which hl2sdk does not ship. To keep the real-SDK
// build linkable, console output goes through IVEngineServer::LogPrint (that
// lands in the server console and the normal log stream) and falls back to
// stderr when no engine interface is available.

#ifndef CRASHDIAG_LOG_H
#define CRASHDIAG_LOG_H

#include "sdk_selector.h"

namespace crashdiag
{
    void SetLogEngine(IVEngineServer* engine);
    void Log(const char* fmt, ...);
}

#endif // CRASHDIAG_LOG_H
