// Common globals shared by the plugin, the crash reporter and the heartbeat
// writer. Reconstructed from the compiled crashdiag.dll.

#ifndef CRASHDIAG_GLOBALS_H
#define CRASHDIAG_GLOBALS_H

#include <cstdint>

// Counters reported on every heartbeat line and in the crash report.
// Field names are taken verbatim from the heartbeat.log format string found
// in the binary:
//   ticks=... map=... players=... lvInit=... lvShut=... clActive=...
//   clDisc=... esCmds=... avgFrameMs=... wsMB=...
struct CrashDiagCounters
{
    uint64_t lvInit      = 0; // LevelInit() calls
    uint64_t lvShut     = 0; // LevelShutdown() calls
    uint64_t clActive   = 0; // ClientActive() calls
    uint64_t clDisc     = 0; // ClientDisconnect() calls
    uint64_t esCmds     = 0; // ClientCommand() calls (ES / ESTools commands)
};

// Snapshot of server state captured when a report is written.
struct CrashDiagSnapshot
{
    char     mapName[256]   = {};
    int      players         = 0;
    int      ticks           = 0;
    float    avgFrameMs     = 0.0f;
    float    wsMB            = 0.0f;
    CrashDiagCounters counters;
};

#endif // CRASHDIAG_GLOBALS_H
