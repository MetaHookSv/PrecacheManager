# PrecacheManager

[English](README.md)

## 导出 GoldSrc / SvEngine 的预缓存资源列表

引擎会把当前地图预缓存的每一个资源记录在 `cl_resourcesonhand` 链表中。PrecacheManager
通过 MetaHook 的 gamedata 符号目录解析该全局量，并注册 `fs_dump_precaches` 控制台命令，
把每一个预缓存的音效、模型和普通文件写入地图同目录下的 `<mapname>.dump.res`。

## 安装

1. 下载并安装 [MetaHookSv](https://github.com/hzqst/MetaHookSv)。

2. 构建或下载 .dll，放到 `/SteamLibrary/steamapps/common/Sven Co-op/svencoop/metahook/plugins` 目录。

3. 在 `/SteamLibrary/steamapps/common/Sven Co-op/svencoop/metahook/configs/plugins.lst` 中新增一行 `PrecacheManager.dll`。

4. 保持 `svencoop/metahook/gamedata/precachemanager` 目录与插件一同分发：其中携带插件加载时解析的 `cl_resourcesonhand` 全局量。

5. 完成。

## 命令

|命令|说明|
|---|---|
|fs_dump_precaches|把当前地图的所有预缓存资源写入 `<mapname>.dump.res`。请在地图内执行该命令。|

## 构建

要求：Windows、Visual Studio 2022、CMake 3.21 或更新版本、Python 3.8 或更新版本。
首次 configure 会把 VC-LTL 5.3.1 下载到 `thirdparty/cache`，并同步 gamedata catalog。

1. 运行 `scripts\build-PrecacheManager-x86-Release.bat`（或 `scripts\build-PrecacheManager-x86-Debug.bat`）。

2. 插件、其 PDB 与 gamedata catalog 会安装到 `install\x86\<Configuration>\svencoop\metahook`。

3. 按「安装」一节所述，把 `PrecacheManager.dll` 和 `gamedata\precachemanager` 复制到 `svencoop/metahook`。

MetaHook SDK 会以固定 commit 自动获取。若要改为基于本地 MetaHook 源码树构建，
可在命令行传入，或在 configure 前导出同名环境变量：

```
scripts\build-PrecacheManager-x86-Release.bat -DMETAHOOK_SOURCE_PATH=D:\MetaHook
```

该路径是提供 `include/metahook.h`、`include/HLSDK` 和 `include/Interface` 的仓库根目录。

传入 `-DPRECACHEMANAGER_SYNC_GAMEDATA=OFF` 可在构建期间跳过 gamedata 下载。
