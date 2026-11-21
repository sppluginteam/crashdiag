// Reconstructed stub of the Source engine "interface" registry.
//
// Against a real HL2SDK this file is replaced by <interface.h> from the SDK.
// The declarations here are name/ABI compatible with the stock HL2SDK headers so
// the same plugin sources compile for both the stub (CI syntax check) and a
// real engine build.

#ifndef CRASHDIAG_STUB_INTERFACE_H
#define CRASHDIAG_STUB_INTERFACE_H

// Source SDK macro: marks a pure-abstract interface class.
#ifndef abstract_class
#define abstract_class class
#endif

typedef void* (*CreateInterfaceFn)(const char* pName, int* pReturnCode);

class IBaseInterface
{
public:
    virtual ~IBaseInterface() {}
};

class InterfaceReg
{
public:
    CreateInterfaceFn m_CreateFn;
    const char* m_pName;
    InterfaceReg* m_pNext;
};

// Linked list head maintained by the stock CreateInterface() implementation.
extern InterfaceReg* g_pInterfaceRegs;

// Stock factory exported by every engine-loaded plugin DLL.
extern "C" void* CreateInterface(const char* pName, int* pReturnCode);

#endif // CRASHDIAG_STUB_INTERFACE_H
