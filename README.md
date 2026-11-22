# crashdiag

A Source engine server **crash-dump diagnostic plugin**. It hooks the engine as an
`IServerPluginCallbacks` plugin (`ISERVERPLUGINCALLBACKS003`), captures fatal
exceptions, writes a text report, a Windows minidump (`.mdmp`) and an exception
classification sidecar (`.cause`), and periodically appends a `heartbeat.log` line
with server state.

[中文说明文档 / Chinese documentation](README.zh-CN.md)

> ⚠️ **Source provenance — read before relying on this code.**
>
> This repository is a **behavior-level reconstruction** of an original compiled
> `crashdiag.dll`. The original source was lost; it was rebuilt from static
> analysis of the binary (RTTI names, exported symbols, interface version,
> ConVar / ConCommand names and defaults, console messages, heartbeat and report
> format strings, imported `dbghelp` / `KERNEL32` / tier0 symbols) and from the
> captured crash report, minidump and heartbeat logs.
>
> It is **not** a byte-for-byte decompilation. The reconstructed binary is **not
> ABI-identical** to the original `crashdiag.dll`; treat this as a clean-room
> reimplementation that reproduces the public plugin contract and the observed
> output, not a restored copy of the lost source.

## Binary architecture: 32-bit and 64-bit are separate files

One `.dll` / `.so` **cannot** serve both architectures. A PE or ELF file carries a
single machine type in its header (`IMAGE_FILE_MACHINE_I386` vs
`IMAGE_FILE_MACHINE_AMD64`), so the binary must match the server process
(`srcds.exe` 32-bit vs `srcds_win64.exe` 64-bit). Two builds are required:

| Target server | Binary |
| --- | --- |
| `srcds.exe` (32-bit) | `crashdiag.dll` built as Win32 x86 |
| `srcds_win64.exe` (x64) | `crashdiag.dll` built as x64 |
| Linux 32-bit | `crashdiag.so` (i386) |
| Linux 64-bit | `crashdiag.so` (x86_64) |

The captured crash came from a 64-bit server (`srcds_win64.exe`), so this
reconstruction targets x64 first. Ship one archive per architecture (or two
subdirectories) and install the one matching the server; a mismatched binary is
simply rejected by the loader and the plugin never loads.

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
| `crashdiag_heartbeat_interval` | `0` | Seconds between `heartbeat.log` lines (`0` disables heartbeat, the default). |
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

1. Either grab a prebuilt archive from the latest release (extract its
   contents directly into `<mod>/addons/` - no extra folders and no vdf
   editing needed), or build the plugin for your architecture (see below).
2. The installed layout is:

   ```
   <mod>/addons/crashdiag.dll      # Windows (matching 32/64-bit srcds)
   <mod>/addons/crashdiag.so       # Linux (best-effort)
   <mod>/addons/crashdiag.vdf      # references "addons/crashdiag" - leave as-is
   ```

   Heartbeat is off by default (`crashdiag_heartbeat_interval 0`); crash
   capture works regardless.

## Build

### Stub SDK build (no engine SDK required)

```bash
cmake -S . -B build -DCRASHDIAG_STUB_SDK=ON
cmake --build build
```

or `./build.sh`. This is the CI type-check path.

### Real HL2SDK build (engine target)

```bash
./build.sh --sdk-path /path/to/hl2sdk-dods
```

Windows:

```bat
build.bat --sdk-path C:\path\to\hl2sdk-dods
```

### DoDS (Day of Defeat: Source)

The CI `build-dods` job clones the `dods` branch of `alliedmodders/hl2sdk` and
attempts a real-SDK x64 build. Because the Win32-only capture path (dbghelp,
process memory, exception classification) is guarded by `#if defined(_WIN32)`,
the POSIX build keeps the plugin entry points but reports dump capture as
unavailable.

## Repository layout

```
src/                          plugin, crash reporter, heartbeat, interface entry
sdk/stub/                     Minimal name/ABI-compatible stub headers (CI syntax check)
CMakeLists.txt                Stub and real-SDK build modes
build.sh / build.bat          Convenience one-shot build scripts
crashdiag.vdf                 Engine plugin manifest
.github/workflows/build.yml   Stub type-check + best-effort DoDS build
```

## Caveats

- The Windows build ignores the benign debug-string pseudo-exceptions
  (`0x40010006` / `0x40010007` from `OutputDebugString`, plus `DBG_CONTROL_C/BREAK`).
  Valve / Steam servers emit these during startup; treating them as crashes made
  an un-debugged server write a 200MB+ full-memory dump and exit with
  `0x40010006` — i.e. a fake "64-bit server crashes on startup". Since v0.1.3
  these are filtered, and the minidump is `DataSegs|HandleData|UnloadedModules|
  ProcessThreadData` rather than `MiniDumpWithFullMemory` (enough for stack/heap
  diagnosis without filling the disk).
- The stub headers declare only the methods the plugin calls. Real engine ABI
  vtable order and full interface signatures must come from the target
  `hl2sdk-*` tree.
- This reconstruction was done without a decompiler; the internal structure may
  differ from the lost original even where the observable output matches.
- The Linux build is a best-effort port; dump capture on non-Windows is a
  stubbed no-op while the heartbeat and report writers still work.
