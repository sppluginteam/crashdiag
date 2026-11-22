// Reconstructed stub of the engine interfaces crashdiag queries.
//   VEngineServer023  -> IVEngineServer
//   VEngineCvar004   -> ICvar (see icvar.h)
//   VProcessUtils001 -> IProcessUtils
//
// Only the methods the plugin actually calls are declared here; a real SDK
// build pulls the full interfaces from hl2sdk/public/.

#ifndef CRASHDIAG_STUB_ENGINE_IFS_H
#define CRASHDIAG_STUB_ENGINE_IFS_H

class IVEngineServer
{
public:
    // Current map name (copied into the crash report / heartbeat line).
    virtual const char* GetMapName() = 0;
    // Number of active players for the report header.
    virtual int GetPlayerCount() = 0;
    // Server tick count at the moment the report/heartbeat is written.
    virtual int GetTickCount() = 0;
    // Command-line basedir / game path helpers.
    virtual const char* GetGameDirectory() = 0;
    // Real SDK signature: copies the absolute mod/game dir into szGetGameDir.
    // Used by the real-SDK build to anchor crash logs under <mod>/.
    virtual void GetGameDir(char* szGetGameDir, int maxlength) = 0;
};

class IProcessUtils
{
public:
    // Reserved for future diagnostics; resolved at Load() like the other two.
    virtual void Reserved() = 0;
};

#endif // CRASHDIAG_STUB_ENGINE_IFS_H
