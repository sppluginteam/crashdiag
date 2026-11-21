// Stub SDK implementations - compiled ONLY for the CRASHDIAG_STUB_SDK build.
// A real engine build links these against the HL2SDK / tier0 / engine libraries
// instead, so this file must not be compiled there.

#ifdef CRASHDIAG_STUB_SDK

#include "icvar.h"
#include "tier0.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// ---------------------------------------------------------------------------
// tier0 console helpers
// ---------------------------------------------------------------------------

static void VMessage(const char* prefix, const char* fmt, va_list args)
{
    if (prefix) std::fputs(prefix, stdout);
    std::vprintf(fmt, args);
}

void ConMsg(const char* fmt, ...)
{
    va_list args; va_start(args, fmt);
    VMessage(nullptr, fmt, args);
    va_end(args);
}

void ConColorMsg(const Color& /*col*/, const char* fmt, ...)
{
    va_list args; va_start(args, fmt);
    VMessage(nullptr, fmt, args);
    va_end(args);
}

void DevWarning(const char* fmt, ...)
{
    va_list args; va_start(args, fmt);
    VMessage("[dev warning] ", fmt, args);
    va_end(args);
}

void Warning(const char* fmt, ...)
{
    va_list args; va_start(args, fmt);
    VMessage("[warning] ", fmt, args);
    va_end(args);
}

void Error(const char* fmt, ...)
{
    va_list args; va_start(args, fmt);
    VMessage("[error] ", fmt, args);
    va_end(args);
}

// ---------------------------------------------------------------------------
// ConCommandBase
// ---------------------------------------------------------------------------

ConCommandBase::ConCommandBase(const char* pName, int flags, const char* pHelpString)
    : m_pName(pName), m_pHelpString(pHelpString), m_flags(flags), m_bRegistered(false)
{
}

ConCommandBase::~ConCommandBase() = default;

const char* ConCommandBase::GetName() const       { return m_pName; }
const char* ConCommandBase::GetHelpText() const  { return m_pHelpString; }
bool ConCommandBase::IsCommand() const           { return false; }
bool ConCommandBase::IsConVar() const            { return false; }

// ---------------------------------------------------------------------------
// CCommand (stub)
// ---------------------------------------------------------------------------

int CCommand::ArgC() const              { return 0; }
const char* CCommand::Arg(int) const    { return ""; }
const char* CCommand::ArgS() const      { return ""; }
const char* CCommand::GetCommandString() const { return ""; }

// ---------------------------------------------------------------------------
// ConCommand
// ---------------------------------------------------------------------------

ConCommand::ConCommand(const char* pName, ConCommandCallbackFn callback,
                       const char* pHelpString, int flags)
    : ConCommandBase(pName, flags, pHelpString), m_fnCallback(callback)
{
}

// ---------------------------------------------------------------------------
// ConVar
// ---------------------------------------------------------------------------

ConVar::ConVar(const char* pName, const char* pDefaultValue, int flags,
               const char* pHelpString, bool bMin, float fMin, bool bMax, float fMax)
    : ConCommandBase(pName, flags, pHelpString),
      m_pDefaultValue(pDefaultValue), m_bMin(bMin), m_bMax(bMax),
      m_fMin(fMin), m_fMax(fMax)
{
    std::strncpy(m_szValue, pDefaultValue ? pDefaultValue : "", sizeof(m_szValue) - 1);
}

const char* ConVar::GetString() const { return m_szValue; }

float ConVar::GetFloat() const
{
    return static_cast<float>(std::atof(m_szValue));
}

int ConVar::GetInt() const
{
    return std::atoi(m_szValue);
}

bool ConVar::GetBool() const
{
    return GetInt() != 0 || std::strcmp(m_szValue, "1") == 0;
}

void ConVar::SetValue(const char* value)
{
    if (!value) return;
    std::strncpy(m_szValue, value, sizeof(m_szValue) - 1);
}

void ConVar::SetValue(float value)
{
    std::snprintf(m_szValue, sizeof(m_szValue), "%f", value);
}

#endif // CRASHDIAG_STUB_SDK
