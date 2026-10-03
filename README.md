# PrecacheManager

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

# Build

Requirements: Windows, Visual Studio 2022, CMake 3.21 or newer, Python 3.8 or newer. The first configure downloads VC-LTL 5.3.1 into `thirdparty/cache` and synchronizes the gamedata catalog.

1. Run `scripts\build-PrecacheManager-x86-Release.bat` (or `scripts\build-PrecacheManager-x86-Debug.bat`).

2. The plugin, its PDB and the gamedata catalog are installed to `install\x86\<Configuration>\svencoop\metahook`.

3. Copy `PrecacheManager.dll` and `gamedata\precachemanager` into `svencoop/metahook`, as described in Install.

The MetaHook SDK is fetched automatically at a pinned commit. To build against a local MetaHook source tree instead, pass it on the command line or export the same environment variable before configuring:

```
scripts\build-PrecacheManager-x86-Release.bat -DMETAHOOK_SOURCE_PATH=D:\MetaHook
```

The path is the repository root that provides `include/metahook.h`, `include/HLSDK` and `include/Interface`.

Pass `-DPRECACHEMANAGER_SYNC_GAMEDATA=OFF` to build without downloading gamedata.
