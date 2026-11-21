// Stub of the tier0 console message helpers used by the plugin.
// Real SDK: tier0/utlvector? No - these live in tier0.dll and are declared in
// the SDK's tier0 headers. The stub provides matching prototypes so the same
// source compiles without the engine tier0 library.

#ifndef CRASHDIAG_STUB_TIER0_H
#define CRASHDIAG_STUB_TIER0_H

#include <cstdarg>

struct Color { int r, g, b, a; };

void ConMsg(const char* fmt, ...);
void ConColorMsg(const Color& col, const char* fmt, ...);
void DevWarning(const char* fmt, ...);
void Warning(const char* fmt, ...);
void Error(const char* fmt, ...);

#endif // CRASHDIAG_STUB_TIER0_H
