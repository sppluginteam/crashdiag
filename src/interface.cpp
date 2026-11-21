// DLL entry points: the engine-facing CreateInterface factory, the interface
// registration list, and the exported `cvar` global.
//
// The compiled binary exports two symbols:
//   CreateInterface              - standard Source interface factory
//   cvar                         - ICvar pointer filled/used by the engine
//
// The plugin advertises itself under ISERVERPLUGINCALLBACKS003.

#include "plugin.h"

#include <cstring>

#ifndef IFACE_OK
#define IFACE_OK     0
#define IFACE_FAILED 1
#endif

InterfaceReg* g_pInterfaceRegs = nullptr;

// Exported engine cvar interface (resolved by the engine / plugin).
ICvar* cvar = nullptr;

static void* CreateCrashDiagPlugin(const char* /*pName*/, int* /*pReturnCode*/)
{
    return &CCrashDiagPlugin::Instance();
}

// Register our single interface. Linked ahead of DllMain so the list head is
// valid before CreateInterface is first called.
static InterfaceReg s_crashDiagReg = {
    &CreateCrashDiagPlugin,
    "ISERVERPLUGINCALLBACKS003",
    nullptr
};

namespace
{
    void RegisterSelf()
    {
        s_crashDiagReg.m_pNext = g_pInterfaceRegs;
        g_pInterfaceRegs = &s_crashDiagReg;
    }
}

extern "C" void* CreateInterface(const char* pName, int* pReturnCode)
{
    for (InterfaceReg* cur = g_pInterfaceRegs; cur != nullptr; cur = cur->m_pNext)
    {
        if (std::strcmp(cur->m_pName, pName) == 0)
        {
            if (pReturnCode) *pReturnCode = IFACE_OK;
            return cur->m_CreateFn(pName, pReturnCode);
        }
    }

    if (pReturnCode) *pReturnCode = IFACE_FAILED;
    return nullptr;
}

#if defined(_WIN32)
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*reserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
        RegisterSelf();
    return TRUE;
}
#else
namespace
{
    struct AutoRegister { AutoRegister() { RegisterSelf(); } };
    static AutoRegister s_autoRegister;
}
#endif
