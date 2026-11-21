// IServerPluginCallbacks (ISERVERPLUGINCALLBACKS003), declared locally.
//
// The hl2sdk `dods` branch does NOT ship this declaration (there is no
// public/IServerPlugin.h and eiface.h only has IVEngineServer), but every
// engine-level plugin needs it. This header mirrors the SDK 2013 revision 003
// vtable exactly, so the same interface can be served by the engine.
//
// If your SDK already provides it, compile with -DCRASHDIAG_SDK_HAS_ISERVERPLUGIN
// and the SDK header will be used instead.

#ifndef CRASHDIAG_COMPAT_ISERVERPLUGIN_H
#define CRASHDIAG_COMPAT_ISERVERPLUGIN_H

#include <interface.h>

struct edict_t;
class CCommand;

#define ISERVERPLUGINCALLBACKS_VERSION_2 "ISERVERPLUGINCALLBACKS002"
#define ISERVERPLUGINCALLBACKS_VERSION_3 "ISERVERPLUGINCALLBACKS003"

abstract_class IServerPluginCallbacks
{
public:
    virtual bool Load(CreateInterfaceFn interfaceFactory,
                      CreateInterfaceFn gameServerFactory) = 0;
    virtual void Unload() = 0;
    virtual void Pause() = 0;
    virtual void UnPause() = 0;
    virtual const char* GetPluginDescription() = 0;
    virtual void LevelInit(char const* pMapName) = 0;
    virtual void ServerActivate(edict_t* pEdictList, int edictCount, int clientMax) = 0;
    virtual void GameFrame(bool simulating) = 0;
    virtual void LevelShutdown() = 0;
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
};

#endif // CRASHDIAG_COMPAT_ISERVERPLUGIN_H
