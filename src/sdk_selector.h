// Selects between the bundled stub SDK (CI syntax check) and a real HL2SDK
// (engine build). Set -DCRASHDIAG_STUB_SDK=ON for the stub build.
#ifndef CRASHDIAG_SDK_SELECTOR_H
#define CRASHDIAG_SDK_SELECTOR_H

#ifdef CRASHDIAG_STUB_SDK
  #include "interface.h"
  #include "eiface.h"
  #include "icvar.h"
  #include "engine_ifs.h"
  #include "tier0.h"
#else
  // Include order matters: eiface.h defines PLUGIN_RESULT, which the local
  // plugin-callback declaration below uses.
  #include <interface.h>
  #include <eiface.h>
  #include <command.h>
  #include <edict.h>
  #ifdef CRASHDIAG_SDK_HAS_ISERVERPLUGIN
    #include <IServerPlugin.h>
  #else
    // hl2sdk `dods` has no plugin-callback declaration of its own.
    #include "sdk/compat/iserverplugin.h"
  #endif
  #include <icvar.h>
#endif

#endif // CRASHDIAG_SDK_SELECTOR_H
