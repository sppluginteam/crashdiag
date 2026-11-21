// Reconstructed stub of eiface.h (IServerPluginCallbacks).
// Real HL2SDK: public/eiface.h. The binary exports ISERVERPLUGINCALLBACKS003.

#ifndef CRASHDIAG_STUB_EIFACE_H
#define CRASHDIAG_STUB_EIFACE_H

#include "interface.h"

struct edict_t;
class CCommand;

abstract_class IServerPluginCallbacks
{
public:
    // Factory handed to Load(): the main engine factory and the game/server
    // factory. Mirrors the HL2SDK Load() signature.
    virtual bool Load(CreateInterfaceFn interfaceFactory,
                      CreateInterfaceFn gameServerFactory) = 0;
    virtual void Unload() = 0;
    virtual void Pause() = 0;
    virtual void UnPause() = 0;
    virtual const char* GetPluginDescription() = 0;

    // Map / server lifetime.
    virtual void LevelInit(const char* pchMapName) = 0;
    virtual void ServerActivate(edict_t* pEdictList, int edictCount, int clientMax) = 0;
    virtual void GameFrame(bool simulating) = 0;
    virtual void LevelShutdown() = 0;

    // Client lifecycle.
    virtual void ClientConnect(edict_t* pEntity, const char* pszName,
                               const char* pszAddress, char* reject, int maxrejectlen) = 0;
    virtual void ClientActive(edict_t* pEntity) = 0;
    virtual void ClientDisconnect(edict_t* pEntity) = 0;
    virtual void ClientPutInServer(edict_t* pEntity, char const* playername) = 0;
    virtual void ClientCommand(edict_t* pEntity, const CCommand& args) = 0;
    virtual void ClientSettingsChanged(edict_t* pEdict) = 0;

    // Cvar query / SteamID validation hooks (present in newer interface revisions).
    virtual bool ProcessQueryCvarValue(edict_t* pEntity, const char* pCvarName,
                                       int queryID, const char* pCvarValue) = 0;
    virtual void NetworkIDValidated(const char* pszName, const char* pszNetworkID) = 0;
};

#endif // CRASHDIAG_STUB_EIFACE_H
