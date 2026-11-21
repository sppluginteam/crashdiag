// Periodic heartbeat writer. Appends one line to <logdir>/heartbeat.log.
// Line format is preserved verbatim from the compiled binary:
//   YYYY-MM-DD HH:MM:SS | ticks=... map=... players=... lvInit=...
//   lvShut=... clActive=... clDisc=... esCmds=... avgFrameMs=... wsMB=...

#ifndef CRASHDIAG_HEARTBEAT_H
#define CRASHDIAG_HEARTBEAT_H

#include "crashdiag_globals.h"

namespace crashdiag
{

// Working set in megabytes (K32GetProcessMemoryInfo on Windows, 0 elsewhere).
float CurrentWorkingSetMB();

// Format and append a heartbeat line for the given snapshot.
void WriteHeartbeat(const CrashDiagSnapshot& snapshot);

} // namespace crashdiag

#endif // CRASHDIAG_HEARTBEAT_H
