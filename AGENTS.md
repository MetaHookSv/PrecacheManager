# AGENTS.md - PrecacheManager Project Guide

## Project Overview

**PrecacheManager** is a utility plugin for MetaHookSV that dumps the current map's precached resource list. The engine tracks every resource the map has precached in the `cl_resourcesonhand` list; the plugin resolves that global through the MetaHook gamedata symbol catalog, registers the `fs_dump_precaches` console command, and writes the exported names to `<mapname>.dump.res` next to the map.

The plugin is tiny and self-contained: three translation units, one gamedata record, no UI, no test suite. Read the sources directly instead of hunting for more documents.

- **Project type**: Native C++ plugin (Windows DLL), MSVC x86 only
- **Engine**: GoldSrc / SvEngine
- **Framework**: MetaHookSV Plugin API (`IPluginsV4`, API 109 or newer)
- **Main dependencies**: MetaHook SDK (public API + `include/HLSDK/common/interface.cpp`). **No third-party library is linked** — no Capstone, no image or HTTP library

## Project Structure

```
PrecacheManager/
├── src/
│   ├── plugins.cpp            # IPluginsV4 lifecycle: Init / LoadEngine / LoadClient / ExitGame
│   ├── plugins.h              # Shared globals, MHPluginName, Sys_Error wrapper, unused search macros
│   ├── privatehook.cpp        # Gamedata resolution of the cl_resourcesonhand global
│   ├── privatehook.h          # resource_t / resourcetype_t layout, GamedataResolvePtr
│   ├── exportfuncs.cpp        # HUD_Init takeover, fs_dump_precaches command, the dump itself
│   └── exportfuncs.h          # HUD_Init declaration; shared cl_enginefunc_t gEngfuncs instance
│                              # (unlike HeapPatch there is no enginedef.h; <metahook.h> is included directly)
├── cmake/
│   ├── Sources.cmake          # Explicit compile list
│   ├── Dependencies.cmake     # Source-path resolution and FetchContent fallback
│   ├── LaunchGame.cmake       # Optional F5 deploy support
│   └── VCLTL.cmake            # VC-LTL 5.3.1
├── scripts/
│   ├── build-PrecacheManager-x86-{Debug,Release}.bat
│   ├── manifests/precachemanager.json  # Gamedata manifest (one `global` symbol)
│   ├── sync-gamedata.py                # Prunes the upstream catalog into the build tree
│   └── validate-gamedata.py            # Validates it before the plugin target builds
├── thirdparty/cache/          # Ignored VC-LTL binary cache
├── build/x86/<configuration>/    # Ignored build output
├── install/x86/<configuration>/  # Ignored install output
├── README.md, README.zh-CN.md # Bilingual install / command / build documentation
└── CMakeLists.txt             # Windows MSVC x86 build and install rules
```

## Core Modules

### 1. Plugin lifecycle (`src/plugins.cpp`)

`IPluginsV4` exported through `EXPOSE_SINGLE_INTERFACE(IPluginsV4, IPluginsV4, METAHOOK_PLUGIN_API_VERSION_V4)`:

- `Init`: stores the host API / interface / engine save
- `LoadEngine`: collects the file system (`FileSystem`, else `FileSystem_HL25`), engine type, buildnum and `g_EngineDLLInfo.ImageBase`, copies `cl_enginefunc_t` into `gEngfuncs`, then runs `Engine_FillAddress()`
- `LoadClient`: copies the export table into `gExportfuncs` and then **takes over the slot**: `pExportFunc->HUD_Init = HUD_Init`. This is a replacement, not an inline hook
- `ExitGame` / `Shutdown`: empty
- `GetVersion`: returns the build timestamp baked in by CMake

### 2. Gamedata resolution (`src/privatehook.cpp`)

- `GamedataResolvePtr(moduleBase, moduleName, symbolName, kind)` wraps `g_pMetaHookAPI->ResolveGameSymbol`
- A miss is fatal: `ReportSymbolFailure` prints the symbol, its owning module, the engine buildnum, the module CRC64 (the engine identity even for unrecognized builds) and the host's status string through `Sys_Error`. `moduleName` only labels the diagnostic; the lookup itself uses `moduleBase`
- `Engine_FillAddress_CL_ResourceOnHand` resolves `engine` / `cl_resourcesonhand` with `MH_GAMESYMBOL_KIND_GLOBAL` and assigns the returned address **directly** to `cl_resourcesonhand`. The resolved global *is* the circular list sentinel node, so no extra dereference is needed

### 3. Command implementation (`src/exportfuncs.cpp`)

`HUD_Init` calls the original `gExportfuncs.HUD_Init()` first, then registers `fs_dump_precaches` via `gEngfuncs.pfnAddCommand`.

`FS_Dump_Precaches`:

1. `gEngfuncs.pfnGetLevelName()` must return a non-empty map name, otherwise it prints a warning and returns
2. The output name is the map name **minus its last four characters** plus `.dump.res`, i.e. the code implicitly assumes a `.bsp` suffix
3. `FILESYSTEM_ANY_OPEN(filename, "wt")`; an open failure only prints an error
4. Walk the circular list starting at `(*cl_resourcesonhand).pNext`, stopping when the walk returns to the sentinel. For each node with `ucFlags & RES_PRECACHED`, write the file name plus `\n` for `t_sound`, `t_model` and `t_generic` only
5. `t_model` entries whose name starts with `*` are skipped (engine-internal brush/inline models)
6. `FILESYSTEM_ANY_CLOSE` and a success message

`t_skin`, `t_decal`, `t_eventscript` and `t_world` are never exported.

### 4. Resource layout (`src/privatehook.h`)

```cpp
typedef struct resource_s
{
    char              szFileName[64];   // File name to download/precache
    resourcetype_t    type;             // t_sound, t_skin, t_model, t_decal, t_generic, ...
    int               nIndex;           // For t_decals
    int               nDownloadSize;
    unsigned char     ucFlags;          // RES_PRECACHED is the filter this plugin uses
    unsigned char     rgucMD5_hash[16];
    unsigned char     playernum;
    unsigned char     rguc_reserved[32];
    struct resource_s *pNext;
    struct resource_s *pPrev;
} resource_t;
```

`private_funcs_t` (a single `CL_PrecacheResources` pointer), `gPrivateFuncs` and the signature-search / hook macros in `src/plugins.h` are retained from the original project but form no complete hook path and are unused by the current version. Do not build on them.

## Key Code Flow

```
Host loader: CreateInterface V4 + Init
    ↓
IPluginsV4::LoadEngine
    ↓
Copy engine funcs, file system, buildnum, engine image base
    ↓
Engine_FillAddress → Engine_FillAddress_CL_ResourceOnHand
    ↓
ResolveGameSymbol("cl_resourcesonhand", MH_GAMESYMBOL_KIND_GLOBAL)
    ├── OK  → assign the sentinel address directly to cl_resourcesonhand
    └── any other status → Sys_Error (symbol / module / buildnum / CRC64 / reason)
    ↓
IPluginsV4::LoadClient → pExportFunc->HUD_Init = HUD_Init
    ↓
HUD_Init: call the original HUD_Init, then pfnAddCommand("fs_dump_precaches", ...)
    ↓
FS_Dump_Precaches
    ↓
pfnGetLevelName → "<map>.bsp" minus the last 4 chars + ".dump.res"
    ↓
Walk (*cl_resourcesonhand).pNext until the sentinel
    ↓
Keep RES_PRECACHED nodes of type t_sound / t_model (no "*") / t_generic
    ↓
Write "maps/<map>.dump.res" and close the handle
```

## Build Instructions

Requirements: Windows, Visual Studio 2022, CMake 3.21 or newer, Python 3.8 or newer, MSVC x86 (`-A Win32`), static CRT (`MultiThreaded`), `_MBCS` and VC-LTL 5.3.1. No `CMAKE_CXX_STANDARD` is set, so the MSVC default language level applies.

```bat
scripts\build-PrecacheManager-x86-Release.bat
scripts\build-PrecacheManager-x86-Debug.bat
```

The scripts configure, build and install. Debug compiles at `/W0`; Release compiles at `/W3` with `/wd4311 /wd4312 /wd4819 /wd4996` suppressed and enables interprocedural optimization plus `/OPT:REF /OPT:ICF`. Output stays in `build/x86/<configuration>/`; the DLL, its PDB and the gamedata catalog are installed to `install/x86/<configuration>/svencoop/metahook/`. Nothing is deployed to the game automatically.

### Dependencies

- **MetaHook SDK**: fetched automatically at a pinned commit; pass `-DMETAHOOK_SOURCE_PATH=D:\MetaHook` or export the same environment variable to build against a local tree. The path is the repository root providing `include/metahook.h`, `include/HLSDK` and `include/Interface`; `cmake/Dependencies.cmake` validates that `include/metahook.h`, `include/HLSDK/common/interface.cpp`, `include/HLSDK/common/cvardef.h` and `include/Interface/IPlugins.h` exist before anything is downloaded
- **VC-LTL 5.3.1**: downloaded once into `thirdparty/cache`
- **No third-party library is linked.** MetaHook is a read-only build input

Keep `cmake/Sources.cmake` as the explicit compile list; `include/HLSDK/common/interface.cpp` is compiled in because `EXPOSE_SINGLE_INTERFACE` (which exports `CreateInterface`) lives there.

### gamedata

`scripts/manifests/precachemanager.json` declares exactly one symbol — `engine` / `cl_resourcesonhand` as a `global` record — for the supported game versions.

`scripts/manifests/precachemanager.json` → `scripts/sync-gamedata.py` → pruned catalog under `build/x86/<Configuration>/assets/svencoop/metahook/gamedata/precachemanager`, validated by `scripts/validate-gamedata.py` before the plugin target builds (`PrecacheManagerGameData` → `PrecacheManagerGameDataValidate` → `PrecacheManager`). Disable with `-DPRECACHEMANAGER_SYNC_GAMEDATA=OFF`. When gamedata usage changes, update the manifest in the same change.

### Optional F5 debugging

```powershell
cmake -S . -B build/launch -G "Visual Studio 17 2022" -A Win32 -DMETAHOOKSV_ENABLE_LAUNCH_GAME=ON
```

Select **LaunchGame** and press F5; **DeployGame** builds, stages and copies the plugin DLL/PDB/resources into an existing MetaHook installation before the debugger attaches. The feature defaults OFF. See `README.md` for `METAHOOKSV_GAME_*` options.

## Engine Compatibility

A single gamedata record (`cl_resourcesonhand`) must exist for the running engine build:

| Game build | Support |
| --- | --- |
| `hl-3248`, `hl-3266`, `hl-3329`, `hl-3647`, `hl-4554` | ✅ |
| `hl-6153` | ✅ |
| `hl-8684`, `hl-10210` | ✅ |
| `svencoop-8948`, `svencoop-10257` | ✅ |
| `cof-5936` (Cry of Fear) | ✅ |
| Any build absent from the catalog | ❌ fatal, never a silent no-op |

Catalog coverage is not a correctness statement: a listed build only means the record exists, not that a dump was verified in-game.

## Important Constants and Macros

```cpp
static_assert(METAHOOK_API_VERSION >= 109, ...);  // ResolveGameSymbol requires MetaHook API 109
#define MHPluginName "PrecacheManager"
#define Sys_Error(msg, ...) g_pMetaHookAPI->SysError("[" MHPluginName "] " msg, __VA_ARGS__);

// List node filter and the exported types (src/privatehook.h)
RES_PRECACHED                                    // ucFlags bit the walk keeps
t_sound = 0, t_skin, t_model, t_decal, t_generic, t_eventscript, t_world

// Uniform file-system access, whichever interface LoadEngine picked
FILESYSTEM_ANY_OPEN(path, mode)
FILESYSTEM_ANY_WRITE(buffer, size, handle)
FILESYSTEM_ANY_CLOSE(handle)
```

Runtime configuration: `PrecacheManager.dll` must be listed in the host's `metahook/configs/plugins.lst`, and `metahook/gamedata/precachemanager` must stay next to it.

## Debugging Tips

1. **Console output**: `Con_Printf` reports the "not in an active map" case, an open failure and the final output path; `Sys_Error` reports a gamedata miss with full diagnostics
2. **Breakpoint locations**: `Engine_FillAddress_CL_ResourceOnHand()` (resolution), `HUD_Init()` (command registration), `FS_Dump_Precaches()` (the walk)
3. **Output location**: `<mapname>.dump.res` is written next to the map, i.e. under `maps/` with the working directory of the game

## Repository Rules

- Preserve the MetaHook API, plugin exports and calling conventions. Match the naming, indentation and comment style of the files you touch
- The engine global comes only from gamedata and the host gamedata contract. **Do not** reintroduce signature search, hard-coded offsets or RVA/VA conversion for it; the search and hook macros left in `src/plugins.h` are unused legacy helpers, not a supported path. When the gamedata lookup fails, report it loudly (as `Sys_Error` does today) instead of adding a fallback
- Keep the `static_assert(METAHOOK_API_VERSION >= 109)` guard in sync with the APIs actually used
- Do not modify external or third-party sources; MetaHook is a read-only build input
- MSVC x86 only. Keep the static CRT / VC-LTL and warning-level settings in `CMakeLists.txt` in sync with the other standalone plugin repositories
- `README.md` and `README.zh-CN.md` are a pair: keep the command name, install steps and build options consistent in both

## Related Links

- **MetaHookSV**: https://github.com/hzqst/MetaHookSv
- **Gamedata symbol catalog**: https://hlnd2t.github.io/GoldSrc_VibeSignatures/
