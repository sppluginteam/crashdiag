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

#ifndef IFACE_OK
#define IFACE_OK     0
#define IFACE_FAILED 1
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
