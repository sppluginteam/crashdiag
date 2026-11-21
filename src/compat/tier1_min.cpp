// Minimal, layout-faithful implementations of the ConVar/ConCommand classes for
// the real-SDK build.
//
// Why this exists: hl2sdk ships only headers. tier1.lib (which normally provides
// these out-of-line member bodies) and tier0.lib are not in the repo, so linking
// the public classes as-is ends in unresolved externals. Everything the engine
// touches - class layout, vtable order, field offsets - is fixed by
// public/tier1/convar.h, and the engine only holds a pointer and calls through
// the vtable/fields declared there. Supplying the same declarations' bodies
// locally therefore keeps the objects ABI-identical to what tier1.dll would have
// produced for this SDK revision.
//
// Only what the plugin uses is implemented meaningfully; the rest is inert but
// defined so the vtables are complete.

#ifndef CRASHDIAG_STUB_SDK

#include "plugin.h"

#include <cstdlib>
#include <cstring>

// Static state normally owned by tier1.
ConCommandBase*          ConCommandBase::s_pConCommandBases = nullptr;
IConCommandBaseAccessor* ConCommandBase::s_pAccessor       = nullptr;

namespace
{
    char* DupString(const char* s)
    {
        if (!s) return nullptr;
        size_t n = std::strlen(s) + 1;
        char* p = static_cast<char*>(std::malloc(n));
        if (p) std::memcpy(p, s, n);
        return p;
    }
}

// ---------------------------------------------------------------------------
// ConCommandBase
// ---------------------------------------------------------------------------

ConCommandBase::ConCommandBase(void)
    : m_pNext(nullptr), m_bRegistered(false), m_pszName(nullptr),
      m_pszHelpString(nullptr), m_nFlags(0)
{
}

ConCommandBase::ConCommandBase(const char* pName, const char* pHelpString, int flags)
    : m_pNext(nullptr), m_bRegistered(false), m_pszName(nullptr),
      m_pszHelpString(nullptr), m_nFlags(0)
{
    CreateBase(pName, pHelpString, flags);
}

ConCommandBase::~ConCommandBase(void)
{
    std::free(const_cast<char*>(m_pszName));
    std::free(const_cast<char*>(m_pszHelpString));
    m_pszName = nullptr;
    m_pszHelpString = nullptr;
}

void ConCommandBase::CreateBase(const char* pName, const char* pHelpString, int flags)
{
    m_pszName = DupString(pName ? pName : "");
    m_pszHelpString = DupString(pHelpString ? pHelpString : "");
    m_nFlags = flags | FCVAR_UNREGISTERED;
    // Link into this module's own chain, mirroring tier1's behaviour.
    m_pNext = s_pConCommandBases;
    s_pConCommandBases = this;
}

char* ConCommandBase::CopyString(const char* from)
{
    return DupString(from);
}

bool ConCommandBase::IsCommand(void) const           { return false; }
bool ConCommandBase::IsFlagSet(int flag) const       { return (m_nFlags & flag) != 0; }
void ConCommandBase::AddFlags(int flags)             { m_nFlags |= flags; }
const char* ConCommandBase::GetName(void) const      { return m_pszName ? m_pszName : ""; }
const char* ConCommandBase::GetHelpText(void) const  { return m_pszHelpString ? m_pszHelpString : ""; }
bool ConCommandBase::IsRegistered(void) const        { return m_bRegistered; }
CVarDLLIdentifier_t ConCommandBase::GetDLLIdentifier() const { return 0; }

const ConCommandBase* ConCommandBase::GetNext(void) const { return m_pNext; }
ConCommandBase* ConCommandBase::GetNext(void)             { return m_pNext; }

void ConCommandBase::Init(void)      { m_bRegistered = true; }
void ConCommandBase::Shutdown(void)  { m_bRegistered = false; }

// ---------------------------------------------------------------------------
// ConCommand
// ---------------------------------------------------------------------------

ConCommand::ConCommand(const char* pName, FnCommandCallbackVoid_t callback,
                       const char* pHelpString, int flags,
                       FnCommandCompletionCallback completionFunc)
    : ConCommandBase(pName, pHelpString, flags)
{
    m_fnCommandCallbackV1        = callback;
    m_fnCompletionCallback       = completionFunc;
    m_bHasCompletionCallback     = completionFunc != nullptr;
    m_bUsingNewCommandCallback   = false;
    m_bUsingCommandCallbackInterface = false;
}

ConCommand::ConCommand(const char* pName, FnCommandCallback_t callback,
                       const char* pHelpString, int flags,
                       FnCommandCompletionCallback completionFunc)
    : ConCommandBase(pName, pHelpString, flags)
{
    m_fnCommandCallback          = callback;
    m_fnCompletionCallback       = completionFunc;
    m_bHasCompletionCallback     = completionFunc != nullptr;
    m_bUsingNewCommandCallback   = true;
    m_bUsingCommandCallbackInterface = false;
}

ConCommand::ConCommand(const char* pName, ICommandCallback* pCallback,
                       const char* pHelpString, int flags,
                       ICommandCompletionCallback* pCommandCompletionCallback)
    : ConCommandBase(pName, pHelpString, flags)
{
    m_pCommandCallback               = pCallback;
    m_pCommandCompletionCallback     = pCommandCompletionCallback;
    m_bHasCompletionCallback         = pCommandCompletionCallback != nullptr;
    m_bUsingNewCommandCallback       = false;
    m_bUsingCommandCallbackInterface = true;
}

ConCommand::~ConCommand(void)
{
}

bool ConCommand::IsCommand(void) const { return true; }

int ConCommand::AutoCompleteSuggest(const char* /*partial*/, CUtlVector<CUtlString>& /*commands*/)
{
    return 0;
}

bool ConCommand::CanAutoComplete(void)
{
    return m_bHasCompletionCallback;
}

void ConCommand::Dispatch(const CCommand& command)
{
    if (m_bUsingCommandCallbackInterface)
    {
        if (m_pCommandCallback) m_pCommandCallback->CommandCallback(command);
    }
    else if (m_bUsingNewCommandCallback)
    {
        if (m_fnCommandCallback) m_fnCommandCallback(command);
    }
    else if (m_fnCommandCallbackV1)
    {
        m_fnCommandCallbackV1();
    }
}

// ---------------------------------------------------------------------------
// ConVar
// ---------------------------------------------------------------------------

ConVar::ConVar(const char* pName, const char* pDefaultValue, int flags)
{
    Create(pName, pDefaultValue, flags, 0);
}

ConVar::ConVar(const char* pName, const char* pDefaultValue, int flags,
               const char* pHelpString)
{
    Create(pName, pDefaultValue, flags, pHelpString);
}

ConVar::ConVar(const char* pName, const char* pDefaultValue, int flags,
               const char* pHelpString, bool bMin, float fMin, bool bMax, float fMax)
{
    Create(pName, pDefaultValue, flags, pHelpString, bMin, fMin, bMax, fMax);
}

ConVar::ConVar(const char* pName, const char* pDefaultValue, int flags,
               const char* pHelpString, FnChangeCallback_t callback)
{
    Create(pName, pDefaultValue, flags, pHelpString, false, 0.0f, false, 0.0f,
           false, 0.0f, false, 0.0f, callback);
}

ConVar::ConVar(const char* pName, const char* pDefaultValue, int flags,
               const char* pHelpString, bool bMin, float fMin, bool bMax, float fMax,
               FnChangeCallback_t callback)
{
    Create(pName, pDefaultValue, flags, pHelpString, bMin, fMin, bMax, fMax,
           false, 0.0f, false, 0.0f, callback);
}

ConVar::ConVar(const char* pName, const char* pDefaultValue, int flags,
               const char* pHelpString, bool bMin, float fMin, bool bMax, float fMax,
               bool bCompMin, float fCompMin, bool bCompMax, float fCompMax,
               FnChangeCallback_t callback)
{
    Create(pName, pDefaultValue, flags, pHelpString, bMin, fMin, bMax, fMax,
           bCompMin, fCompMin, bCompMax, fCompMax, callback);
}

ConVar::~ConVar(void)
{
    std::free(m_pszString);
    std::free(const_cast<char*>(m_pszDefaultValue));
    m_pszString = nullptr;
    m_pszDefaultValue = nullptr;
}

void ConVar::Create(const char* pName, const char* pDefaultValue, int flags,
                    const char* pHelpString, bool bMin, float fMin, bool bMax, float fMax,
                    bool bCompMin, float fCompMin, bool bCompMax, float fCompMax,
                    FnChangeCallback_t callback)
{
    CreateBase(pName, pHelpString, flags);

    m_pParent            = this;
    m_pszDefaultValue    = DupString(pDefaultValue ? pDefaultValue : "");
    m_pszString          = DupString(pDefaultValue ? pDefaultValue : "");
    m_StringLength       = static_cast<int>(std::strlen(m_pszString ? m_pszString : ""));
    m_fValue             = static_cast<float>(std::atof(pDefaultValue ? pDefaultValue : "0"));
    m_nValue             = std::atoi(pDefaultValue ? pDefaultValue : "0");

    m_bHasMin            = bMin;
    m_fMinVal            = fMin;
    m_bHasMax            = bMax;
    m_fMaxVal            = fMax;
    m_fnChangeCallback   = callback;

    m_bHasCompMin        = bCompMin;
    m_fCompMinVal        = fCompMin;
    m_bHasCompMax        = bCompMax;
    m_fCompMaxVal        = fCompMax;
    m_bCompetitiveRestrictions = bCompMin || bCompMax;
}

void ConVar::Create_Vtbl(const char* pName, const char* pDefaultValue, int flags,
                         const char* pHelpString, bool bMin, float fMin,
                         bool bMax, float fMax, FnChangeCallback_t callback)
{
    Create(pName, pDefaultValue, flags, pHelpString, bMin, fMin, bMax, fMax,
           false, 0.0f, false, 0.0f, callback);
}

bool ConVar::IsFlagSet(int flag) const              { return (m_pParent->m_nFlags & flag) != 0; }
const char* ConVar::GetHelpText(void) const         { return ConCommandBase::GetHelpText(); }
bool ConVar::IsRegistered(void) const               { return ConCommandBase::IsRegistered(); }
const char* ConVar::GetName(void) const             { return ConCommandBase::GetName(); }
void ConVar::AddFlags(int flags)                    { m_pParent->m_nFlags |= flags; }
bool ConVar::IsCommand(void) const                  { return false; }

void ConVar::Init(void)
{
    ConCommandBase::Init();
}

bool ConVar::ClampValue(float& value)
{
    if (m_pParent->m_bHasMin && value < m_pParent->m_fMinVal)
    {
        value = m_pParent->m_fMinVal;
        return true;
    }
    if (m_pParent->m_bHasMax && value > m_pParent->m_fMaxVal)
    {
        value = m_pParent->m_fMaxVal;
        return true;
    }
    return false;
}

void ConVar::ChangeStringValue(const char* /*tempVal*/, float /*flOldValue*/)
{
    // This plugin installs no change callbacks.
}

void ConVar::InternalSetValue(const char* value)
{
    float fNew = static_cast<float>(std::atof(value ? value : "0"));
    ClampValue(fNew);

    m_pParent->m_fValue = fNew;
    m_pParent->m_nValue = static_cast<int>(fNew);

    std::free(m_pParent->m_pszString);
    m_pParent->m_pszString = DupString(value ? value : "");
    m_pParent->m_StringLength =
        static_cast<int>(std::strlen(m_pParent->m_pszString ? m_pParent->m_pszString : ""));
}

void ConVar::InternalSetFloatValue(float fNewValue)
{
    InternalSetFloatValue2(fNewValue, true);
}

void ConVar::InternalSetIntValue(int nValue)
{
    InternalSetFloatValue2(static_cast<float>(nValue), true);
}

void ConVar::InternalSetFloatValue2(float fNewValue, bool /*bForce*/)
{
    ClampValue(fNewValue);
    m_pParent->m_fValue = fNewValue;
    m_pParent->m_nValue = static_cast<int>(fNewValue);
}

void ConVar::SetValue(const char* value)
{
    InternalSetValue(value);
}

void ConVar::SetValue(float value)
{
    InternalSetFloatValue2(value, true);
}

void ConVar::SetValue(int value)
{
    InternalSetFloatValue2(static_cast<float>(value), true);
}

void ConVar::InstallChangeCallback(FnChangeCallback_t callback)
{
    m_pParent->m_fnChangeCallback = callback;
}

void ConVar::Revert(void)
{
    InternalSetValue(m_pParent->m_pszDefaultValue);
}

bool ConVar::GetMin(float& minVal) const
{
    if (!m_pParent->m_bHasMin) return false;
    minVal = m_pParent->m_fMinVal;
    return true;
}

bool ConVar::GetMax(float& maxVal) const
{
    if (!m_pParent->m_bHasMax) return false;
    maxVal = m_pParent->m_fMaxVal;
    return true;
}

const char* ConVar::GetDefault(void) const
{
    return m_pParent->m_pszDefaultValue;
}

void ConVar::SetDefault(const char* pszDefault)
{
    std::free(const_cast<char*>(m_pParent->m_pszDefaultValue));
    m_pParent->m_pszDefaultValue = DupString(pszDefault ? pszDefault : "");
}

bool ConVar::GetCompMin(float& minVal) const
{
    if (!m_pParent->m_bHasCompMin) return false;
    minVal = m_pParent->m_fCompMinVal;
    return true;
}

bool ConVar::GetCompMax(float& maxVal) const
{
    if (!m_pParent->m_bHasCompMax) return false;
    maxVal = m_pParent->m_fCompMaxVal;
    return true;
}

bool ConVar::SetCompetitiveMode(bool bCompetitive)
{
    m_pParent->m_bCompetitiveRestrictions = bCompetitive;
    return true;
}

#endif // !CRASHDIAG_STUB_SDK
