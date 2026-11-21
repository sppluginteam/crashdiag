// Reconstructed stub of icvar.h / convar classes.
// Real HL2SDK: public/icvar.h, public/convar.h, public/command.h.

#ifndef CRASHDIAG_STUB_ICVAR_H
#define CRASHDIAG_STUB_ICVAR_H

#include <cstddef>

// ConVar/ConCommand flags (subset used by crashdiag).
#define FCVAR_NONE                 0
#define FCVAR_UNREGISTERED         (1 << 0)
#define FCVAR_DEVELOPMENTONLY      (1 << 1)
#define FCVAR_GAMEDLL              (1 << 2)
#define FCVAR_CLIENTDLL            (1 << 3)
#define FCVAR_DO_NOT_RECORD        (1 << 4)
#define FCVAR_ONLY_IN_SERVER       (1 << 5)
#define FCVAR_REPLICATED           (1 << 6)
#define FCVAR_CHEAT                (1 << 7)
#define FCVAR_SPONLY               (1 << 8)
#define FCVAR_PRINTABLEONLY        (1 << 9)
#define FCVAR_UNLOGGED             (1 << 10)
#define FCVAR_NOEXTRAWHITEPACE     (1 << 11)
#define FCVAR_NEVER_AS_STRING      (1 << 12)
#define FCVAR_PROTECTED            (1 << 13)
#define FCVAR_SPONLY_ONLY_IN_SP    (1 << 14)
#define FCVAR_ARCHIVE              (1 << 15)
#define FCVAR_NOTIFY               (1 << 16)
#define FCVAR_USERINFO             (1 << 17)
#define FCVAR_BATCHED              (1 << 18)
#define FCVAR_USERINFO_CHANGED_BY_USER (1 << 19)
#define FCVAR_SERVER_CAN_EXECUTE   (1 << 20)
#define FCVAR_CLIENTCMD_CAN_EXECUTE (1 << 21)

class IConCommandBaseAccessor
{
public:
    virtual bool RegisterConCommandBase(class ConCommandBase* pCommand) = 0;
};

class ConCommandBase
{
public:
    ConCommandBase(const char* pName, int flags, const char* pHelpString);
    virtual ~ConCommandBase();

    const char* GetName() const;
    const char* GetHelpText() const;
    bool IsCommand() const;
    bool IsConVar() const;

protected:
    const char* m_pName;
    const char* m_pHelpString;
    int m_flags;
    bool m_bRegistered;
};

class CCommand
{
public:
    int ArgC() const;
    const char* Arg(int nArg) const;
    const char* ArgS() const;
    const char* GetCommandString() const;
};

typedef void (*ConCommandCallbackFn)(const CCommand& args);

class ConCommand : public ConCommandBase
{
public:
    ConCommand(const char* pName, ConCommandCallbackFn callback,
               const char* pHelpString, int flags = FCVAR_NONE);
    ConCommandCallbackFn m_fnCallback;
};

class ConVar : public ConCommandBase
{
public:
    ConVar(const char* pName, const char* pDefaultValue, int flags = FCVAR_NONE,
           const char* pHelpString = nullptr,
           bool bMin = false, float fMin = 0.0f,
           bool bMax = false, float fMax = 0.0f);

    const char* GetString() const;
    float GetFloat() const;
    int GetInt() const;
    bool GetBool() const;
    void SetValue(const char* value);
    void SetValue(float value);

private:
    const char* m_pDefaultValue;
    mutable char m_szValue[256];
    bool m_bMin, m_bMax;
    float m_fMin, m_fMax;
};

// ICvar interface (subset). Real HL2SDK: public/icvar.h.
class ICvar
{
public:
    virtual void RegisterConCommandBase(ConCommandBase* pCommand) = 0;
    virtual ConVar* FindVar(const char* pName) = 0;
};

// Exported by the plugin DLL and filled by the engine (matches the `cvar`
// export seen in the compiled binary).
extern ICvar* cvar;

#endif // CRASHDIAG_STUB_ICVAR_H
