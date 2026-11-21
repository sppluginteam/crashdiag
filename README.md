# crashdiag

A Source engine server **crash-dump diagnostic plugin**. It hooks the engine as an
`IServerPluginCallbacks` plugin (`ISERVERPLUGINCALLBACKS003`), captures fatal
exceptions, writes a text report, a Windows minidump (`.mdmp`) and an exception
classification sidecar (`.cause`), and periodically appends a `heartbeat.log` line
with server state.

> ⚠️ **Source provenance — read before relying on this code.**
>
> This repository is a **behavior-level reconstruction** of an original compiled
> `crashdiag.dll`. The original source was lost; it was rebuilt from static
> analysis of the binary (RTTI names, exported symbols, interface version,
> ConVar / ConCommand names and defaults, console messages, heartbeat and report
> format strings, imported `dbghelp` / `KERNEL32` / tier0 symbols) and from the
> captured crash report, minidump and heartbeat logs.
>
> It is **not** a byte-for-byte decompilation. The control flow, helper
> decomposition and internal data layout are reconstructed to match the observed
> behavior. The reconstructed binary is **not ABI-identical** to the original
> `crashdiag.dll`; treat this as a clean-room reimplementation that reproduces the
> public plugin contract and the observed output, not a restored copy of the
> lost source.

## What the recovered binary told us

| Evidence | Value |
| --- | --- |
| Plugin type | Engine-level `IServerPluginCallbacks` (not a SourceMod extension, not Metamod) |
| Interface version | `ISERVERPLUGINCALLBACKS003` |
| Engine interfaces resolved | `VEngineServer023`, `VEngineCvar004`, `VProcessUtils001` |
| Exported symbols | `CreateInterface`, `cvar` |
| RTTI class | `CCrashDiagPlugin : IServerPluginCallbacks` |
| Description | `crashdiag - independent ES/ESTools crash diagnostic (capture + log)` |

### ConVars

| ConVar | Default | Description |
| --- | --- | --- |
| `crashdiag_enabled` | `1` | Master switch for crash capture and heartbeat. |
| `crashdiag_logdir` | `crashdiag` | Subdirectory under `<mod>/` where logs are written. |
| `crashdiag_heartbeat_interval` | `60` | Seconds between `heartbeat.log` lines (`0` disables heartbeat). |
| `crashdiag_enable_test` | `0` | When `1`, `crashdiag_test_crash` may actually crash. |

### Console command

| Command | Description |
| --- | --- |
| `crashdiag_test_crash` | Intentionally trigger a fatal crash to verify the capture pipeline. Guarded by `crashdiag_enable_test`. |

### Output files

All files are written under `<mod>/<crashdiag_logdir>/`:

- `heartbeat.log` — one line per heartbeat interval:
  `YYYY-MM-DD HH:MM:SS | ticks=... map=... players=... lvInit=... lvShut=... clActive=... clDisc=... esCmds=... avgFrameMs=... wsMB=...`
- `crash_YYYYMMDD_HHMMSS.log` — fatal exception report (time, executable, map,
  players, ticks, counters, exception code/address, registers, call stack,
  loaded modules).
- `crash_YYYYMMDD_HHMMSS.mdmp` — Windows minidump (via `dbghelp!MiniDumpWriteDump`).
- `crash_YYYYMMDD_HHMMSS.cause` — classification label sidecar.

Exception labels cover `EXCEPTION_ACCESS_VIOLATION`, stack overflow, illegal /
privileged instruction, integer / floating-point divide by zero, integer
overflow, in-page error, breakpoint, the C++ EH code `0xE06D7363`,
`STATUS_FATAL_APP_EXIT` (abort / `std::terminate`),
`STATUS_STACK_BUFFER_OVERRUN` (`__fastfail` / GS / terminate),
`STATUS_HEAP_CORRUPTION`, and a fallback `Unknown exception`.

## Install

1. Build the plugin for your server platform (see below).
2. Copy the resulting `crashdiag.dll` / `libcrashdiag.so` next to the plugin
   loader, and the `crashdiag.vdf` manifest:

   ```
   <mod>/addons/crashdiag.dll      # Windows x64
   <mod>/addons/crashdiag.so       # Linux (best-effort)
   <mod>/addons/crashdiag.vdf
   ```

   The bundled `crashdiag.vdf` registers the plugin as `addons/crashdiag`.

## Build

Two build modes are provided; the same sources compile against either the bundled
stub headers or a real HL2SDK.

### Stub SDK build (no engine SDK required)

Used by CI for a fast syntax / type check. Compiles against `sdk/stub/`.

```bash
cmake -S . -B build -DCRASHDIAG_STUB_SDK=ON
cmake --build build
```

or simply:

```bash
./build.sh
```

### Real HL2SDK build (engine target)

Point CMake at a checked-out HL2SDK whose `public/` tree has `eiface.h`,
`icvar.h`, `interface.h`:

```bash
./build.sh --sdk-path /path/to/hl2sdk-dods
```

Windows:

```bat
build.bat --sdk-path C:\path\to\hl2sdk-dods
```

### DoDS (Day of Defeat: Source)

The original captured server was a DoDS-class Source server (`srcds_win64.exe`,
map `de_dust`). The CI `build-dods` job clones the `dods` branch of
`alliedmodders/hl2sdk` and attempts a real-SDK compile. Because the Win32-only
capture path (dbghelp, process memory, exception classification) is guarded by
`#if defined(_WIN32)`, the POSIX build keeps the plugin entry points but reports
dump capture as unavailable.

## Repository layout

```
src/
  plugin.cpp / plugin.h        CCrashDiagPlugin - IServerPluginCallbacks implementation
  crash_report.cpp / .h        Handlers, text report, minidump, classification
  heartbeat.cpp / .h           Periodic heartbeat.log writer
  interface.cpp                CreateInterface factory, interface registration, `cvar` export
  crashdiag_globals.h          Shared snapshot / counter structs
  sdk_selector.h               Stub vs. real SDK header selection
sdk/stub/                      Minimal name/ABI-compatible stub headers (CI syntax check)
CMakeLists.txt                 Stub and real-SDK build modes
build.sh / build.bat           Convenience one-shot build scripts
crashdiag.vdf                  Engine plugin manifest
```

## Caveats

- The stub headers in `sdk/stub/` declare only the methods the plugin calls.
  Real engine ABI vtable order and full interface signatures must be taken from
  the target `hl2sdk-*` tree.
- This reconstruction was done without a decompiler; the internal structure may
  differ from the lost original even where the observable output matches.
- The Linux build is a best-effort port. Crash dump capture on non-Windows is a
  stubbed no-op; the heartbeat and report writers still work.
