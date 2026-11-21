// Reconstructed stub of eiface.h (IServerPluginCallbacks).
// Real HL2SDK: public/eiface.h. The binary exports ISERVERPLUGINCALLBACKS003.
//
// The vtable order below mirrors SDK 2013 revision 003 exactly, including the
// revision-2/3 tail (OnQueryCvarValueFinished, OnEdictAllocated, OnEdictFreed),
// so the stub build exercises the same declaration shape the real SDK uses.

#ifndef CRASHDIAG_STUB_EIFACE_H
#define CRASHDIAG_STUB_EIFACE_H

#include "interface.h"

struct edict_t;
class CCommand;

// PLUGIN_RESULT lives in eiface.h/iserverplugin.h in the real SDK; every
// callback that can steer the engine returns it.
enum PLUGIN_RESULT
{
    PLUGIN_CONTINUE = 0,
    PLUGIN_OVERRIDE,
    PLUGIN_STOP
};

// Only the type names are needed by the callback signatures below.
typedef int QueryCvarCookie_t;
typedef int EQueryCvarValueStatus;

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
    virtual void ClientActive(edict_t* pEntity) = 0;
    virtual void ClientDisconnect(edict_t* pEntity) = 0;
    virtual void ClientPutInServer(edict_t* pEntity, char const* playername) = 0;
    virtual void SetCommandClient(int index) = 0;
    virtual void ClientSettingsChanged(edict_t* pEdict) = 0;
    virtual PLUGIN_RESULT ClientConnect(bool* bAllowConnect, edict_t* pEntity,
                                        const char* pszName, const char* pszAddress,
                                        char* reject, int maxrejectlen) = 0;
    virtual PLUGIN_RESULT ClientCommand(edict_t* pEntity, const CCommand& args) = 0;
    virtual PLUGIN_RESULT NetworkIDValidated(const char* pszName,
                                             const char* pszNetworkID) = 0;

    // Revision 2/3 tail.
    virtual void OnQueryCvarValueFinished(QueryCvarCookie_t iCookie, edict_t* pPlayerEntity,
                                          EQueryCvarValueStatus eStatus, const char* pCvarName,
                                          const char* pCvarValue) = 0;
    virtual void OnEdictAllocated(edict_t* edict) = 0;
    virtual void OnEdictFreed(const edict_t* edict) = 0;
};

#endif // CRASHDIAG_STUB_EIFACE_H
