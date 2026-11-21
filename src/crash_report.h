// Crash capture: vectored + unhandled exception handlers, text report,
// minidump (.mdmp) and classification (.cause) writer.
//
// The original crashdiag.dll links dbghelp.dll (MiniDumpWriteDump, StackWalk64,
// SymInitialize, SymFromAddr, SymGetModuleBase64, SymFunctionTableAccess64,
// SymGetLineFromAddr64, SymSetOptions) and KERNEL32 (RtlCaptureContext,
// K32GetProcessMemoryInfo). Those are Win32 APIs; the POSIX build keeps the
// same entry points but reports that dump capture is unavailable.

#ifndef CRASHDIAG_REPORT_H
#define CRASHDIAG_REPORT_H

#include "crashdiag_globals.h"

namespace crashdiag
{

// Install the vectored handler and the unhandled-exception filter.
// Returns false if the handlers could not be registered.
bool InstallCrashHandler();

// Remove whatever handlers InstallCrashHandler() registered.
void RemoveCrashHandler();

// Called by the plugin every frame so the reporter always has a current
// snapshot to attach to a report.
void UpdateSnapshot(const CrashDiagSnapshot& snapshot);

// Current snapshot (copied under the reporter's own lock when needed).
CrashDiagSnapshot CurrentSnapshot();

// Build the log directory (<mod>/<logdir>) if it is missing.
void EnsureLogDirectory(const char* logDir);

// Current configured log directory (heartbeat writer shares it).
const char* LogDir();

// Produce crash_YYYYMMDD_HHMMSS.{log,mdmp,cause} inside the log directory.
// `reason` overrides the auto-classified exception description when given.
void WriteCrashReport(const char* reason);

} // namespace crashdiag

#endif // CRASHDIAG_REPORT_H
