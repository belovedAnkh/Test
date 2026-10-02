# SexLabSceneFreeLook 2.0

适用于 **Skyrim SE / AE 1.6.1170** 的 SKSE 插件（CommonLibSSE-NG），配合 **Improved Camera SE 1.1.2** 使用。
目的：**SexLab 场景开始时如果你在第一人称，场景里自动用角色自己的眼睛看，直到场景结束；平时什么都不改。**

## 前提

- Improved Camera 的配置（NEFARAM 用的是 `Profiles\110.ini`）里 `[EVENTS] bScripted=1`。其余保持整合包原样即可，`bThirdPerson` 不用开。
- SexLab Framework（`SexLab.esm`）。没装时插件什么都不做。

## 它做什么

只在下面条件**同时**成立时介入：玩家在 SexLab 某个线程（`SexLabThread00`～`14`）的演员别名里，而且 SexLab 还没把玩家解锁（`SexLabAnimatingFaction` 等级不是 0）。

1. SexLab 把镜头从第一人称强制切到第三人称的那一刻（`ThirdPersonState::Begin`，上一个状态是第一人称）：
   - 打开 `PlayerControls::data.povScriptMode`（等同于 `Game.DisablePlayerControls` 里 abCamSwitch=true 的效果）。Improved Camera 把它当成「脚本场景」，于是从第一帧起就按场景处理，后面谁再打开移动控制（例如 SLSO 的进度条脚本每秒一次 `EnablePlayerControls`）都不会再把镜头踢回原生第一人称。
   - 把第三人称缩放目标设成 `fMinCurrentZoom`（最近一档）。Improved Camera 只有在最近一档时才切到眼睛视角，所以场景一开始就是眼睛视角。
   - 对话菜单开着时不介入（Improved Camera 本身在这种情况下也不接管）。
2. 场景进行中：保持 `povScriptMode`，保持 `freeRotationEnabled`（鼠标只转视角，不转身体）。
3. SexLab 解锁玩家（`UnlockActor` 把等级设为 0）或玩家离开线程别名：如果 `povScriptMode` 是本插件打开的，就关掉它。Improved Camera 随后自己切回第一人称。

场景里用鼠标滚轮拉远就是第三人称，滚回最近又回到眼睛视角。`povScriptMode` 打开期间，原版的切换视角键不可用。

开场时在第三人称的场景，本插件不介入，和原来一样。

## 为什么需要它（Improved Camera 1.1.2 源码）

- SexLab 1.66 在 `sslActorAlias.ClearEffects()` 里先强制第三人称，之后 `LockActor()` 才关移动控制。Improved Camera 会先看到一帧「普通第三人称」，在 `bThirdPerson=0` 时就放弃这一场的眼睛视角。
- Improved Camera 一看到移动控制被重新打开，就判定场景结束；如果当时是眼睛视角，会调 `ForceFirstPerson` 进原生第一人称（站立高度、身体僵直）。
- 脚本强制切第三人称时，Improved Camera 不会替你拉近（骑马、骑龙会），游戏会恢复你平时的第三人称距离。

## 日志

`Documents\My Games\Skyrim Special Edition\SKSE\SexLabSceneFreeLook.log`：载入、找到的 SexLab 表单、每次接管和释放各一行。

## 安装 / 停用

`SKSE\Plugins\SexLabSceneFreeLook.dll`。停用：在 MO2 里关掉这个 mod。不写存档数据。

## 编译

Windows、MSVC、CMake 3.21+、Ninja、vcpkg：

```
set VCPKG_ROOT=C:\path\to\vcpkg
cmake --preset release
cmake --build --preset release
```

GitHub Actions（`.github/workflows/build.yml`）在 `windows-latest` 上编译，DLL 作为 artifact `SexLabSceneFreeLook` 上传。
