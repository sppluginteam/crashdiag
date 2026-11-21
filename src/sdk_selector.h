// Selects between the bundled stub SDK (CI syntax check) and a real HL2SDK
// (engine build). Set -DCRASHDIAG_STUB_SDK=ON for the stub build.
//
// Real-SDK note: hl2sdk `dods` has no public/IServerPlugin.h, but
// public/eiface.h pulls in public/engine/iserverplugin.h, which *does* declare
// ISERVERPLUGINCALLBACKS003 (PLUGIN_RESULT, SetCommandClient,
// OnQueryCvarValueFinished, OnEdictAllocated, OnEdictFreed). Including eiface.h
// therefore gives us the authoritative v003 interface; declaring it again
// alongside the SDK would be a redefinition.
// eiface.h also brings in convar.h (CCommand/ConVar) and edict.h.

#ifndef CRASHDIAG_SDK_SELECTOR_H
#define CRASHDIAG_SDK_SELECTOR_H

#ifdef CRASHDIAG_STUB_SDK
  #include "interface.h"
  #include "eiface.h"
  #include "icvar.h"
  #include "engine_ifs.h"
  #include "tier0.h"
#else
  #include <interface.h>
  #include <eiface.h>
  #include <icvar.h>
#endif

#endif // CRASHDIAG_SDK_SELECTOR_H
