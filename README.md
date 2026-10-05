# PrecacheManager

[中文文档](README.zh-CN.md)

## Dump the GoldSrc / SvEngine precached resource list

The engine tracks every resource the current map has precached in the `cl_resourcesonhand` list. PrecacheManager resolves that global through the MetaHook gamedata symbol catalog and adds the `fs_dump_precaches` console command, which writes every precached sound, model and generic file into `<mapname>.dump.res` next to the map.

# Install

1. Download and install [MetaHookSv](https://github.com/hzqst/MetaHookSv).

2. Build or download .dll, put it into `/SteamLibrary/steamapps/common/Sven Co-op/svencoop/metahook/plugins` directory.

3. Add `PrecacheManager.dll` in `/SteamLibrary/steamapps/common/Sven Co-op/svencoop/metahook/configs/plugins.lst` as a newline.

4. Keep the `svencoop/metahook/gamedata/precachemanager` directory shipped next to the plugin: it carries the `cl_resourcesonhand` global the plugin resolves at load time.

5. Enjoy.

# Command

|Command|Comment|
|---|---|
|fs_dump_precaches|Write every precached resource of the current map into `<mapname>.dump.res`. Run it while connected to a map.|

## F5 debugging (optional)

Install MetaHook and enable this plugin in the game's `plugins.lst` first. Configure a standalone Visual Studio Win32 solution:

```powershell
cmake -S . -B build/launch -G "Visual Studio 17 2022" -A Win32 -DMETAHOOKSV_ENABLE_LAUNCH_GAME=ON
```

Open the solution, select **LaunchGame** and press **F5**. **DeployGame** builds this plugin and its dependencies, stages Install, and copies plugin DLLs/PDBs/resources before the native debugger starts the existing game launcher. Root launchers/runtime files and plugin lists remain unchanged. Set VS to build before running and **Do not launch** on build errors; stop the game before redeploying. Ordinary builds do not deploy.

`METAHOOKSV_GAME_DIRECTORY` defaults to Steam discovery; `METAHOOKSV_GAME_APPID` defaults to `225840`. Set `METAHOOKSV_GAME_MOD` for a custom mod and `METAHOOKSV_GAME_ARGUMENTS` for extra arguments. Debug and Release are supported.

The shared module uses `METAHOOKSV_LAUNCH_GAME_MODULE_DIR`, the surrounding MetaHookSv checkout, or a pinned source archive. Without Installer sources, it downloads the self-contained CLI from GitHub `latest` (no .NET required); `METAHOOKSV_INSTALLER_RELEASE` selects a fixed tag, and `METAHOOKSV_INSTALLER_CLI_EXECUTABLE` supplies an offline EXE. Plugin mode requires v20261004c or later. Valid caches under `build/launch/launch-game/installer/<release>` are reused without update checks; select another tag or clear that private cache to upgrade. `GH_TOKEN`/`GITHUB_TOKEN` may be supplied through the environment if GitHub API rate limits prevent the first download. The feature defaults OFF and performs no extra downloads when disabled.

# Build

Requirements: Windows, Visual Studio 2022, CMake 3.21 or newer, Python 3.8 or newer. The first configure downloads VC-LTL 5.3.1 into `thirdparty/cache` and synchronizes the gamedata catalog.

1. Run `scripts\build-PrecacheManager-x86-Release.bat` (or `scripts\build-PrecacheManager-x86-Debug.bat`).

2. The plugin, its PDB and the gamedata catalog are installed to `install\x86\<Configuration>\svencoop\metahook`.

3. Copy `PrecacheManager.dll` and `gamedata\precachemanager` into `svencoop/metahook`, as described in Install.

The MetaHook SDK is fetched automatically from the latest `main`. To build against a local MetaHook source tree instead, pass it on the command line or export the same environment variable before configuring:

```
scripts\build-PrecacheManager-x86-Release.bat -DMETAHOOK_SOURCE_PATH=D:\MetaHook
```

The path is the repository root that provides `include/metahook.h`, `include/HLSDK` and `include/Interface`.

Pass `-DPRECACHEMANAGER_SYNC_GAMEDATA=OFF` to build without downloading gamedata.
