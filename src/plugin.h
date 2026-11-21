// CCrashDiagPlugin - the engine-level server plugin exposed as
// ISERVERPLUGINCALLBACKS003.
//
// Reconstructed from crashdiag.dll: RTTI names (CCrashDiagPlugin,
// IServerPluginCallbacks), exported interface version, ConVar/ConCommand names
// and defaults, heartbeat/report format strings, and the produced crash
// report / heartbeat log.

#ifndef CRASHDIAG_PLUGIN_H
#define CRASHDIAG_PLUGIN_H

#include "sdk_selector.h"
#include "crashdiag_globals.h"

class CCrashDiagPlugin : public IServerPluginCallbacks
{
public:
    static CCrashDiagPlugin& Instance();

    // IServerPluginCallbacks
    bool Load(CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory) override;
    void Unload() override;
    void Pause() override {}
    void UnPause() override {}
    const char* GetPluginDescription() override;

    void LevelInit(const char* pchMapName) override;
    void ServerActivate(edict_t* pEdictList, int edictCount, int clientMax) override;
    void GameFrame(bool simulating) override;
    void LevelShutdown() override;

    void ClientConnect(edict_t* pEntity, const char* pszName,
                       const char* pszAddress, char* reject, int maxrejectlen) override;
    void ClientActive(edict_t* pEntity) override;
    void ClientDisconnect(edict_t* pEntity) override;
    void ClientPutInServer(edict_t* pEntity, char const* playername) override;
    void ClientCommand(edict_t* pEntity, const CCommand& args) override;
    void ClientSettingsChanged(edict_t* pEdict) override;

    bool ProcessQueryCvarValue(edict_t* pEntity, const char* pCvarName,
                               int queryID, const char* pCvarValue) override;
    void NetworkIDValidated(const char* pszName, const char* pszNetworkID) override;

private:
    CCrashDiagPlugin();

    void BuildSnapshot(CrashDiagSnapshot& snap);
    void RegisterCommands();
    static void TestCrashCmd(const CCommand& args);

    IVEngineServer* m_pEngine    = nullptr;
    ICvar*          m_pCvar      = nullptr;
    IProcessUtils*  m_pProcess  = nullptr;

    // ConVars (defaults recovered from the binary).
    ConVar m_cvEnabled;
    ConVar m_cvLogDir;
    ConVar m_cvInterval;
    ConVar m_cvEnableTest;
    ConCommand m_cmdTestCrash;

    CrashDiagCounters m_counters;
    char m_mapName[256];
    int  m_players = 0;

    // Frame timing (QueryPerformanceCounter based).
    double m_lastFrameQpc   = 0.0;
    float  m_avgFrameMs     = 0.0f;
    double m_lastHeartbeatQpc = 0.0;

    bool m_loaded = false;
};

#endif // CRASHDIAG_PLUGIN_H
