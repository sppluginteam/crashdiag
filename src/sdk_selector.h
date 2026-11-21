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
  #include "interface.h"
  #include "eiface.h"
  #include "icvar.h"
  #include "edict.h"
  #include "command.h"
#endif

#endif // CRASHDIAG_SDK_SELECTOR_H
