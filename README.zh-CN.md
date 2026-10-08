# PrecacheManager

[English](README.md)

## 导出 GoldSrc / SvEngine 的预缓存资源列表

引擎会把当前地图预缓存的每一个资源记录在 `cl_resourcesonhand` 链表中。PrecacheManager
通过 MetaHook 的 gamedata 符号目录解析该全局量，并注册 `fs_dump_precaches` 控制台命令，
把每一个预缓存的音效、模型和普通文件写入地图同目录下的 `<mapname>.dump.res`。

## 安装

1. 下载并安装 [MetaHookSv](https://github.com/MetaHookSv/MetaHookSv)。

2. 构建或下载 .dll，放到 `/SteamLibrary/steamapps/common/Sven Co-op/svencoop/metahook/plugins` 目录。

3. 在 `/SteamLibrary/steamapps/common/Sven Co-op/svencoop/metahook/configs/plugins.lst` 中新增一行 `PrecacheManager.dll`。

4. 保持 `svencoop/metahook/gamedata/precachemanager` 目录与插件一同分发：其中携带插件加载时解析的 `cl_resourcesonhand` 全局量。

5. 完成。

## 命令

|命令|说明|
|---|---|
|fs_dump_precaches|把当前地图的所有预缓存资源写入 `<mapname>.dump.res`。请在地图内执行该命令。|

## F5 调试（可选）

先安装 MetaHook，并在游戏的 `plugins.lst` 中启用本插件，然后配置独立的 Visual Studio Win32 解决方案：

```powershell
cmake -S . -B build/launch -G "Visual Studio 17 2022" -A Win32 -DMETAHOOKSV_ENABLE_LAUNCH_GAME=ON
```

打开解决方案，选择 **LaunchGame** 后按 **F5**。**DeployGame** 编译本插件及依赖，暂存 Install，再复制插件 DLL、PDB 和资源，最后由原生调试器启动已有游戏 launcher。不会修改根目录启动器/运行库或插件列表。VS 应开启运行前构建，并将构建失败策略设为 **不启动**；重新部署前请退出游戏。普通构建不会部署。

`METAHOOKSV_GAME_DIRECTORY` 默认通过 Steam 自动查找，`METAHOOKSV_GAME_APPID` 默认为 `225840`。自定义 mod 使用 `METAHOOKSV_GAME_MOD`，附加参数使用 `METAHOOKSV_GAME_ARGUMENTS`；支持 Debug 和 Release。

共享模块依次从 `METAHOOKSV_LAUNCH_GAME_MODULE_DIR`、所在 MetaHookSv 聚合仓库或固定提交的源码包获取。缺少 Installer 源码时自动下载 GitHub `latest` 的自包含 CLI，无需安装 .NET；可用 `METAHOOKSV_INSTALLER_RELEASE` 固定 tag，或用 `METAHOOKSV_INSTALLER_CLI_EXECUTABLE` 指定离线 EXE。插件模式要求 v20261004c 或之后版本。`build/launch/launch-game/installer/<release>` 下的有效缓存直接复用，不自动升级；切换 tag 或清理该私有缓存后重新下载。首次下载若触发 GitHub API 限流，可通过环境变量 `GH_TOKEN`/`GITHUB_TOKEN` 提供凭据。功能默认 OFF，关闭时不新增下载。

## 构建

要求：Windows、Visual Studio 2022、CMake 3.21 或更新版本、Python 3.8 或更新版本。
首次 configure 会把 VC-LTL 5.3.1 下载到 `thirdparty/cache`，并同步 gamedata catalog。

1. 运行 `scripts\build-PrecacheManager-x86-Release.bat`（或 `scripts\build-PrecacheManager-x86-Debug.bat`）。

2. 插件、其 PDB 与 gamedata catalog 会安装到 `install\x86\<Configuration>\svencoop\metahook`。

3. 按「安装」一节所述，把 `PrecacheManager.dll` 和 `gamedata\precachemanager` 复制到 `svencoop/metahook`。

MetaHook SDK 会自动获取最新的 `main` 分支。若要改为基于本地 MetaHook 源码树构建，
可在命令行传入，或在 configure 前导出同名环境变量：

```
scripts\build-PrecacheManager-x86-Release.bat -DMETAHOOK_SOURCE_PATH=D:\MetaHook
```

该路径是提供 `include/metahook.h`、`include/HLSDK` 和 `include/Interface` 的仓库根目录。

传入 `-DPRECACHEMANAGER_SYNC_GAMEDATA=OFF` 可在构建期间跳过 gamedata 下载。

## C/C++ 格式化

使用 [MetaHookSv/FormatValidation](https://github.com/MetaHookSv/FormatValidation)
共享工具及固定版本 **clang-format 23.1.3**，采用 DiligentCore 风格（4 空格，保留
include 顺序）。为 CMake 使用的 Python 解释器安装格式工具：

```sh
python -m pip install clang-format==23.1.3
cmake -S . -B build/format "-DFORMAT_VALIDATION_ONLY=ON"
cmake --build build/format --target format-check
cmake --build build/format --target format
```

格式专用配置需要 CMake 3.21+、Git、Python 3.9+（CI 使用 3.12）及构建生成器；
使用 `-G Ninja` 可无需 Visual Studio。它不准备原生 SDK 或游戏依赖。
格式目标需显式执行，不加入普通 DLL 构建。使用 Visual Studio 生成器时，执行目标
需追加 `--config Debug` 或 `--config Release`。

聚合仓库注入 `FORMAT_VALIDATION_SOURCE_PATH=thirdparty/FormatValidation`。
独立组件支持该 CMake 参数及同名环境变量；为空时通过 FetchContent 获取固定工具
提交。相对路径应加引号，例如
`"-DFORMAT_VALIDATION_SOURCE_PATH=../../thirdparty/FormatValidation"`。
配置时在仓库根目录生成被 gitignore 的 `.clang-format` 供编辑器使用；格式规则应
在共享仓库修改，不修改生成副本。可通过 `FORMAT_VALIDATION_CLANG_FORMAT_EXECUTABLE`
指定工具路径，但版本仍须与固定版本一致。

检查覆盖 `src/`、`include/`、`tests/` 中维护的 C/C++ 文件，包括未被 Git 忽略的新文件。
相对仓库根目录的排除规则位于 `.clang-format-ignore`；第三方源和构建产物不纳入检查。
`clang-format` workflow 在 push、pull request 和手动运行时执行全量检查。
