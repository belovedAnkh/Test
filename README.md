# SexLabSceneFreeLook

适用于 **Skyrim Special Edition / Anniversary Edition 1.6.1170** 的 SKSE 插件（CommonLibSSE-NG，C++）。

## 作用

SexLab 之类的场景脚本会强制切到第三人称并接管玩家控制。在这种场景里，原版会让鼠标同时转动镜头和玩家身体。
本插件让鼠标**只转镜头，不转玩家身体**。

实现方式：钩住 `RE::ThirdPersonState` 的虚函数 `SetFreeRotationMode`（vtable 索引 `0x0D`）。
先调用原函数，然后在下面两个条件**同时**成立时，把 `freeRotationEnabled` 设为 `true`：

- `PlayerControls::data.povScriptMode` 为 `true`
- `ControlMap::IsMovementControlsEnabled()` 为 `false`

其他任何时候都不改动原版行为。插件没有配置文件，也不处理镜头角度限制（例如 Improved Camera 的 ±65° 偏移限制）。

### 关于“第一人称”

装了 Improved Camera 时，场景里切到的“第一人称”在游戏内部仍然是第三人称相机状态（`ThirdPersonState`），
只是镜头移到了头部，所以同一个 hook 对它也有效。原版（未装 Improved Camera）的第一人称使用 `FirstPersonState`，本插件**不处理**。

### 日志

插件会在状态变化时记录一行日志（`povScriptMode`、移动控制是否启用、原版和最终的 `freeRotationEnabled`），
用来确认 hook 在场景里有没有生效。场景里如果日志里完全没有这类记录，说明当时相机不在 `ThirdPersonState`。

## 依赖

- Skyrim AE 1.6.1170
- [SKSE64](https://skse.silverlock.org/)（支持 1.6.1170 的版本）
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)（AE 版本，`version-1-6-1170-0.bin`）

## 安装

把 `SexLabSceneFreeLook.dll` 放到：

```
Data\SKSE\Plugins\
```

用 Mod Organizer 2 或 Vortex 时，打包成 `SKSE\Plugins\SexLabSceneFreeLook.dll` 即可。
日志在 `Documents\My Games\Skyrim Special Edition\SKSE\SexLabSceneFreeLook.log`。

## 停用

删除 `Data\SKSE\Plugins\SexLabSceneFreeLook.dll`，或在 MO2 / Vortex 里禁用这个 mod。
插件不写存档数据，随时可以卸载，不影响存档。

## 编译

需要 Windows、Visual Studio 2022 或更新的 MSVC、CMake 3.21+、Ninja、vcpkg。

```
set VCPKG_ROOT=C:\path\to\vcpkg
cmake --preset release
cmake --build --preset release
```

CommonLibSSE-NG 通过 `cmake/ports/commonlibsse-ng` 里的 vcpkg overlay port 按固定提交构建。
GitHub Actions（`.github/workflows/build.yml`）会在 `windows-latest` 上编译，DLL 作为 artifact `SexLabSceneFreeLook` 上传。
