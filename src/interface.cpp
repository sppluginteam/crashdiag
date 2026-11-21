// DLL entry points.
//
// The compiled binary exports two symbols:
//   CreateInterface              - standard Source interface factory
//   cvar                         - ICvar pointer filled/used by the engine
//
// The plugin advertises itself under ISERVERPLUGINCALLBACKS003.
//
// Two registration paths are required, because the stub SDK and a real HL2SDK
// model interface registration differently:
//
//   * stub SDK  - this file owns the registry list and the exported
//                 CreateInterface() implementation, so CI can link a real .so.
//   * HL2SDK    - public/tier1/interface.h defines InterfaceReg with a
//                 constructor that link-lists the entry into
//                 InterfaceReg::s_pInterfaceRegs, and the engine's tier1 library
//                 provides CreateInterface(). Defining our own here would clash
//                 (LNK2005) with the engine factory, so only a static
//                 InterfaceReg is emitted.

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

#ifdef CRASHDIAG_STUB_SDK

InterfaceReg* g_pInterfaceRegs = nullptr;

static void* CreateCrashDiagPlugin(const char* /*pName*/, int* /*pReturnCode*/)
{
    return CreateCrashDiagPluginInstance();
}

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

#else // real HL2SDK

// HL2SDK's InterfaceReg constructor takes an InstantiateInterfaceFn, i.e. a
// function WITH NO ARGUMENTS, and registers itself with the SDK's global list.
static InterfaceReg s_crashDiagReg(&CreateCrashDiagPluginInstance,
                                  "ISERVERPLUGINCALLBACKS003");

#endif // CRASHDIAG_STUB_SDK
