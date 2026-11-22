#include "plugin.h"
#include "crash_report.h"
#include "heartbeat.h"
#include "log.h"

#include <cstring>

#if defined(_WIN32)
  #include <windows.h>
#else
  #include <time.h>
#endif

namespace
{
    double NowSeconds()
    {
#if defined(_WIN32)
        LARGE_INTEGER count, freq;
        QueryPerformanceCounter(&count);
        QueryPerformanceFrequency(&freq);
        return (double)count.QuadPart / (double)freq.QuadPart;
#else
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (double)ts.tv_sec + (double)ts.tv_nsec / 1.0e9;
#endif
    }
}

CCrashDiagPlugin& CCrashDiagPlugin::Instance()
{
    static CCrashDiagPlugin inst;
    return inst;
}

CCrashDiagPlugin::CCrashDiagPlugin()
    : m_cvEnabled("crashdiag_enabled", "1", FCVAR_NONE,
        "Master switch for crashdiag crash capture and heartbeat."),
      m_cvLogDir("crashdiag_logdir", "crashdiag", FCVAR_NONE,
        "Subdirectory under <mod>/ where crashdiag writes its logs."),
      m_cvInterval("crashdiag_heartbeat_interval", "60", FCVAR_NONE,
        "Seconds between heartbeat.log lines (0 disables heartbeat)."),
      m_cvEnableTest("crashdiag_enable_test", "0", FCVAR_NONE,
        "When 1, the crashdiag_test_crash command is allowed to actually crash."),
      m_cmdTestCrash("crashdiag_test_crash", &CCrashDiagPlugin::TestCrashCmd,
        "Intentionally trigger a fatal crash to verify crashdiag logging. "
        "Guarded by crashdiag_enable_test.", FCVAR_NONE)
{
    std::memset(m_mapName, 0, sizeof(m_mapName));
}

void CCrashDiagPlugin::RegisterCommands()
{
    if (m_pCvar)
    {
        // The dods ICvar names this RegisterConCommand (not
        // RegisterConCommandBase, which only exists on IConCommandBaseAccessor).
        m_pCvar->RegisterConCommand(&m_cvEnabled);
        m_pCvar->RegisterConCommand(&m_cvLogDir);
        m_pCvar->RegisterConCommand(&m_cvInterval);
        m_pCvar->RegisterConCommand(&m_cvEnableTest);
        m_pCvar->RegisterConCommand(&m_cmdTestCrash);
    }
}

bool CCrashDiagPlugin::Load(CreateInterfaceFn interfaceFactory,
                            CreateInterfaceFn /*gameServerFactory*/)
{
    int rc = 0;
    m_pEngine = reinterpret_cast<IVEngineServer*>(interfaceFactory("VEngineServer023", &rc));
    m_pCvar   = reinterpret_cast<ICvar*>(interfaceFactory("VEngineCvar004", &rc));
#ifdef CRASHDIAG_STUB_SDK
    m_pProcess  = reinterpret_cast<IProcessUtils*>(interfaceFactory("VProcessUtils001", &rc));
#endif

    crashdiag::SetLogEngine(m_pEngine);
    RegisterCommands();
    crashdiag::EnsureLogDirectory(m_cvLogDir.GetString());
    crashdiag::InstallCrashHandler();

    m_loaded = true;
    m_lastFrameQpc = NowSeconds();
    m_lastHeartbeatQpc = m_lastFrameQpc;

    crashdiag::Log("[crashdiag] loaded. logdir=%s engine=%p cvar=%p enabled=%d\n",
                   m_cvLogDir.GetString(), static_cast<void*>(m_pEngine),
                   static_cast<void*>(m_pCvar), m_cvEnabled.GetBool() ? 1 : 0);

    // Emit a heartbeat immediately on load. The default 60 s interval is
    // too long to confirm the plugin is actually running, and the very first
    // heartbeat only fires after a full minute has elapsed - so a 30-second
    // smoke test would not catch the directory being created. This line is
    // what a deployer looks for to prove the plugin came up.
    {
        CrashDiagSnapshot snap;
        BuildSnapshot(snap);
        crashdiag::UpdateSnapshot(snap);
        crashdiag::WriteHeartbeat(snap);
    }

    return true;
}

void CCrashDiagPlugin::Unload()
{
    crashdiag::RemoveCrashHandler();
    m_loaded = false;
    crashdiag::Log("[crashdiag] unloaded.\n");
}

const char* CCrashDiagPlugin::GetPluginDescription()
{
    return "crashdiag - independent ES/ESTools crash diagnostic (capture + log)";
}

void CCrashDiagPlugin::BuildSnapshot(CrashDiagSnapshot& snap)
{
    snap = CrashDiagSnapshot{};
    // Everything below is tracked by the plugin itself: IVEngineServer in this
    // SDK revision exposes neither GetMapName/GetPlayerCount nor a tick query.
    std::strncpy(snap.mapName, m_mapName, sizeof(snap.mapName) - 1);
    snap.players    = m_players;
    snap.ticks      = static_cast<int>(m_ticks);
    snap.avgFrameMs = m_avgFrameMs;
    snap.wsMB       = crashdiag::CurrentWorkingSetMB();
    snap.counters   = m_counters;
}

void CCrashDiagPlugin::LevelInit(char const* pMapName)
{
    ++m_counters.lvInit;
    m_players = 0;
    m_ticks = 0;
    if (pMapName) std::strncpy(m_mapName, pMapName, sizeof(m_mapName) - 1);
}

void CCrashDiagPlugin::ServerActivate(edict_t* /*pEdictList*/, int /*edictCount*/,
                                      int /*clientMax*/)
{
}

void CCrashDiagPlugin::GameFrame(bool /*simulating*/)
{
    if (!m_loaded) return;

    ++m_ticks;

    double now = NowSeconds();
    if (m_lastFrameQpc > 0.0)
    {
        double dt = now - m_lastFrameQpc;
        float ms = static_cast<float>(dt * 1000.0);
        // Light exponential moving average, matching the avgFrameMs field.
        m_avgFrameMs = m_avgFrameMs * 0.9f + ms * 0.1f;
    }
    m_lastFrameQpc = now;

    CrashDiagSnapshot snap;
    BuildSnapshot(snap);
    crashdiag::UpdateSnapshot(snap);

    int interval = m_cvInterval.GetInt();
    if (interval > 0 && (now - m_lastHeartbeatQpc) >= interval)
    {
        m_lastHeartbeatQpc = now;
        crashdiag::WriteHeartbeat(snap);
    }
}

void CCrashDiagPlugin::LevelShutdown()
{
    ++m_counters.lvShut;
}

PLUGIN_RESULT CCrashDiagPlugin::ClientConnect(bool* /*bAllowConnect*/, edict_t* /*pEntity*/,
                                             const char* /*pszName*/, const char* /*pszAddress*/,
                                             char* /*reject*/, int /*maxrejectlen*/)
{
    return PLUGIN_CONTINUE;
}

void CCrashDiagPlugin::ClientActive(edict_t* /*pEntity*/)
{
    ++m_counters.clActive;
    ++m_players;
}

void CCrashDiagPlugin::ClientDisconnect(edict_t* /*pEntity*/)
{
    ++m_counters.clDisc;
    if (m_players > 0) --m_players;
}

void CCrashDiagPlugin::ClientPutInServer(edict_t* /*pEntity*/, char const* /*playername*/)
{
}

PLUGIN_RESULT CCrashDiagPlugin::ClientCommand(edict_t* /*pEntity*/, const CCommand& /*args*/)
{
    ++m_counters.esCmds;
    return PLUGIN_CONTINUE;
}

void CCrashDiagPlugin::ClientSettingsChanged(edict_t* /*pEdict*/)
{
}

PLUGIN_RESULT CCrashDiagPlugin::NetworkIDValidated(const char* /*pszName*/,
                                                  const char* /*pszNetworkID*/)
{
    return PLUGIN_CONTINUE;
}

void CCrashDiagPlugin::TestCrashCmd(const CCommand& /*args*/)
{
    CCrashDiagPlugin& self = Instance();

    if (!self.m_cvEnabled.GetBool())
    {
        crashdiag::Log("[crashdiag] disabled (crashdiag_enabled=0); refusing test crash.\n");
        return;
    }
    if (!self.m_cvEnableTest.GetBool())
    {
        crashdiag::Log("[crashdiag] set crashdiag_enable_test 1 before using crashdiag_test_crash.\n");
        return;
    }

    crashdiag::Log("[crashdiag] intentional access violation (null deref) - writing crash log...\n");
    crashdiag::WriteCrashReport("EXCEPTION_ACCESS_VIOLATION");

    // Intentional null dereference to exercise the capture pipeline.
    volatile int* nullPtr = nullptr;
    *nullPtr = 0;
}
