// DLL entry points.
//
// The compiled binary exports two symbols:
//   CreateInterface  - standard Source interface factory
//   cvar             - ICvar pointer the engine looks for
//
// Normally CreateInterface and the InterfaceReg registry come from tier1.lib.
// hl2sdk ships no tier1 library, so the factory and its (tiny) registry live
// here instead: the DLL registers ISERVERPLUGINCALLBACKS003 and answers the
// engine's GetProcAddress("CreateInterface") call.

#include "plugin.h"

#include <cstring>

#if defined(_WIN32)
  #include <windows.h>
#endif

#ifndef IFACE_OK
#define IFACE_OK     0
#define IFACE_FAILED 1
#endif

// DllMain is intentionally SILENT. It must not write any log file and must
// not call OutputDebugStringA: a healthy, non-crashing server produces zero
// files and zero debug events from crashdiag. (Writing a load trace here and
// emitting OutputDebugString were both removed - the latter even looked like
// a crash on un-debugged x64 servers.) All real output happens only when an
// actual crash is captured, or when the heartbeat is explicitly enabled.
#if defined(_WIN32)
BOOL WINAPI DllMain(HINSTANCE /*hinstDLL*/, DWORD /*fdwReason*/, LPVOID /*lpReserved*/)
{
    return TRUE;
}
#endif

// Exported engine cvar interface (resolved by the engine / plugin).
ICvar* cvar = nullptr;

static void* CreateCrashDiagPluginInstance()
{
    return &CCrashDiagPlugin::Instance();
}

namespace
{
    struct InterfaceEntry
    {
        const char* pName;
        void* (*pCreate)();
        InterfaceEntry* pNext;
    };

    InterfaceEntry g_crashDiagEntry = {
        "ISERVERPLUGINCALLBACKS003",
        &CreateCrashDiagPluginInstance,
        nullptr
    };

    InterfaceEntry* g_pInterfaces = &g_crashDiagEntry;
}

extern "C" void* CreateInterface(const char* pName, int* pReturnCode)
{
    for (InterfaceEntry* cur = g_pInterfaces; cur != nullptr; cur = cur->pNext)
    {
        if (cur->pName && pName && std::strcmp(cur->pName, pName) == 0)
        {
            if (pReturnCode) *pReturnCode = IFACE_OK;
            return cur->pCreate();
        }
    }

    if (pReturnCode) *pReturnCode = IFACE_FAILED;
    return nullptr;
}
