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

// DllMain confirms the DLL was actually loaded by the engine and, when the
// engine reports "Unable to load plugin", tells us whether it was rejected
// during DllMain (returning FALSE here makes LoadLibraryEx fail with the
// same message as a missing file). A file-based trace in the crash log dir
// also survives the stdout-buffering quirks observed in prior sessions.
#if defined(_WIN32)
  #include <cstdio>
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    (void)lpReserved;
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        // Open the log dir from the same path the Load() step will use
        // (crashdiag_logdir cvar defaults to "crashdiag", so the CWD at
        // plugin-load time - which the engine sets to <mod>/ - is where the
        // trace goes).
        FILE* f = std::fopen("crashdiag\\_load_trace.log", "a");
        if (f)
        {
            std::fprintf(f, "[crashdiag] DllMain PROCESS_ATTACH %p\n", (void*)hinstDLL);
            std::fclose(f);
        }
        OutputDebugStringA("[crashdiag] DllMain PROCESS_ATTACH - loaded\n");
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        FILE* f = std::fopen("crashdiag\\_load_trace.log", "a");
        if (f)
        {
            std::fprintf(f, "[crashdiag] DllMain PROCESS_DETACH\n");
            std::fclose(f);
        }
        OutputDebugStringA("[crashdiag] DllMain PROCESS_DETACH\n");
    }
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
