# crashdiag

Source 引擎服务器**崩溃转储诊断插件**。以 `IServerPluginCallbacks` 引擎级插件形式挂载（接口版本 `ISERVERPLUGINCALLBACKS003`），捕获致命异常，写出文本报告、Windows minidump（`.mdmp`）与异常分类副文件（`.cause`），并按间隔向 `heartbeat.log` 追加一行服务器状态。

[English README](README.md)

> ⚠️ **源码来源说明（重要）**
>
> 本仓库是对某个已编译 `crashdiag.dll` 的**行为级重建**。原始源码已丢失，重建依据为对二进制的静态分析（RTTI 类名、导出符号、接口版本、ConVar / ConCommand 名称与默认值、控制台消息、心跳与报告格式串、`dbghelp` / `KERNEL32` / tier0 导入符号），以及当时抓到的崩溃报告、minidump 与心跳日志。
>
> 这**不是**逐字节反编译。控制流、辅助函数拆分与内部数据布局是按可观测行为重建的；重建出的二进制与原版 `crashdiag.dll` **不保证 ABI 一致**。请把它当作复现公开插件契约与输出格式的净室实现，而不是找回的原始源码。

## 32 位与 64 位：必须分开编译，单文件不能通吃

一个 `.dll` / `.so` **无法**同时服务 32 位与 64 位。PE / ELF 头里只记录一种机器类型（`IMAGE_FILE_MACHINE_I386` 与 `IMAGE_FILE_MACHINE_AMD64` 互斥），二进制必须与服务端进程位宽一致（`srcds.exe` 32 位 / `srcds_win64.exe` 64 位）。

| 目标服务端 | 需要的产物 |
| --- | --- |
| `srcds.exe`（32 位） | Win32 x86 编译的 `crashdiag.dll` |
| `srcds_win64.exe`（x64） | x64 编译的 `crashdiag.dll` |
| Linux 32 位 | `crashdiag.so`（i386） |
| Linux 64 位 | `crashdiag.so`（x86_64） |

当时的崩溃来自 64 位服务端（`srcds_win64.exe`），因此本仓库优先面向 x64。发布时应按架构分目录或分压缩包，安装时选与服务器一致的那一份；位宽不匹配时加载器会直接拒绝，插件根本不会加载。

## 从二进制中确认的事实

| 证据 | 值 |
| --- | --- |
| 插件类型 | 引擎级 `IServerPluginCallbacks`（非 SourceMod 扩展，非 Metamod 插件） |
| 接口版本 | `ISERVERPLUGINCALLBACKS003` |
| 获取的引擎接口 | `VEngineServer023`、`VEngineCvar004`、`VProcessUtils001` |
| 导出符号 | `CreateInterface`、`cvar` |
| RTTI 类 | `CCrashDiagPlugin : IServerPluginCallbacks` |
| 插件描述 | `crashdiag - independent ES/ESTools crash diagnostic (capture + log)` |

### ConVar

| ConVar | 默认值 | 说明 |
| --- | --- | --- |
| `crashdiag_enabled` | `1` | 崩溃捕获与心跳的总开关 |
| `crashdiag_logdir` | `crashdiag` | 日志子目录（位于 `<mod>/` 下） |
| `crashdiag_heartbeat_interval` | `0` | 心跳行间隔秒数（0 关闭心跳，默认值） |
| `crashdiag_enable_test` | `0` | 置 1 后 `crashdiag_test_crash` 才允许真的崩溃 |

### 控制台命令

| 命令 | 说明 |
| --- | --- |
| `crashdiag_test_crash` | 故意触发致命崩溃以验证捕获链路；受 `crashdiag_enable_test` 保护 |

### 输出文件

全部写入 `<mod>/<crashdiag_logdir>/`：

- `heartbeat.log`：每个心跳间隔一行
  `YYYY-MM-DD HH:MM:SS | ticks=... map=... players=... lvInit=... lvShut=... clActive=... clDisc=... esCmds=... avgFrameMs=... wsMB=...`
- `crash_YYYYMMDD_HHMMSS.log`：致命异常报告，含时间、进程、地图、玩家数、tick、计数器、异常码/地址、寄存器、调用栈、已加载模块。
- `crash_YYYYMMDD_HHMMSS.mdmp`：Windows minidump（`dbghelp!MiniDumpWriteDump`）。
- `crash_YYYYMMDD_HHMMSS.cause`：分类标签副文件。

异常分类覆盖 `EXCEPTION_ACCESS_VIOLATION`、栈溢出、非法/特权指令、整数/浮点除零、整数溢出、页内错误、断点、C++ EH（`0xE06D7363`）、`STATUS_FATAL_APP_EXIT`（abort / `std::terminate`）、`STATUS_STACK_BUFFER_OVERRUN`（`__fastfail` / GS / terminate）、`STATUS_HEAP_CORRUPTION`，以及兜底的 `Unknown exception`。

## 安装

1. 按服务端位宽编译对应产物（见上表）。
2. 把二进制与清单放进 mod 目录：

   ```
   <mod>/addons/crashdiag.dll      # Windows（与 srcds 位宽一致）
   <mod>/addons/crashdiag.so       # Linux（尽力支持）
   <mod>/addons/crashdiag.vdf
   ```

## 编译

### 桩 SDK 编译（无需引擎 SDK）

```bash
cmake -S . -B build -DCRASHDIAG_STUB_SDK=ON
cmake --build build
```

或直接 `./build.sh`。CI 的类型检查走这条路径。

### 真实 HL2SDK 编译

```bash
./build.sh --sdk-path /path/to/hl2sdk-dods
```

Windows：

```bat
build.bat --sdk-path C:\path\to\hl2sdk-dods
```

### DoDS（Day of Defeat: Source）

CI 的 `build-dods` 作业会克隆 `alliedmodders/hl2sdk` 的 `dods` 分支，尝试真实 SDK 的 x64 编译。由于 Win32 专有的捕获代码（dbghelp、进程内存、异常分类）全部包在 `#if defined(_WIN32)` 守卫内，POSIX 构建保留插件入口但转储捕获为空实现。

## 仓库结构

```
src/                          插件、崩溃报告、心跳、接口入口
sdk/stub/                     名称/ABI 兼容的最小桩头文件（CI 语法检查用）
CMakeLists.txt                桩 SDK 与真实 SDK 两种构建模式
build.sh / build.bat          一键构建脚本
crashdiag.vdf                 引擎插件清单
.github/workflows/build.yml   桩类型检查 + 尽力而为的 DoDS 构建
```

## 已知限制

- Windows 版会忽略 `OutputDebugString` 产生的伪异常（`0x40010006` / `0x40010007`，
  以及 `DBG_CONTROL_C/BREAK`）。Valve / Steam 服务器在启动阶段会发出这些事件；
  如果把它们当崩溃处理，在没有调试器附加时会写出 200MB+ 的完整内存转储并让进程
  以 `0x40010006` 退出——表现就是"64 位服务器一启动就崩溃"。v0.1.3 起这些事件
  被过滤，minidump 也从 `MiniDumpWithFullMemory` 改为
  `DataSegs|HandleData|UnloadedModules|ProcessThreadData`（足够分析栈和堆损坏，
  又不会填满磁盘）。
- 桩头文件只声明插件实际调用的方法；真实引擎 ABI 的 vtable 顺序与完整接口签名必须取自目标 `hl2sdk-*`。
- 本次重建没有反编译器参与，即便可观测输出一致，内部结构也可能与丢失的原版不同。
- Linux 构建为尽力支持；非 Windows 下转储捕获为空实现，心跳与报告写入仍然可用。
