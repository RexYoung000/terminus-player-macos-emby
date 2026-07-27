# Terminus Player macOS & Emby 适配版

Terminus Player 的个人维护分支，面向现代 macOS 持续修复和优化，并以兼容
当前 Emby 服务器为主要目标。项目使用内嵌网页界面浏览媒体库，通过 MPV
完成本地视频播放。

> 当前处于 `1.8.0-dev` 适配阶段：macOS 启动、网页来源、Emby Quick
> Connect 能力判断和过时 MPV 参数已经完成源码修复及自动构建验证；
> 账号认证、媒体库和完整播放链路仍需在真实服务器上完成界面验收。
> 请以本文的兼容状态为准，不将开发目标理解为已完成承诺。

## 项目初衷

[Terminus Media Player](https://github.com/Terminus-Media/jellyfin-media-player)
曾提供支持 macOS、Windows 和 Linux 的开源桌面播放器，但现有 macOS
安装包发布较早，在现代 macOS 和新版本媒体服务器上逐渐出现兼容问题：

- 原 macOS 应用未经开发者签名和苹果公证。
- 现有安装包只有 Intel `x86_64` 架构，在 Apple Silicon 上依赖 Rosetta 2。
- 启动逻辑会尝试写入 `/usr/local/etc/fonts`，在现代 macOS 上可能直接崩溃。
- 内置网页客户端版本较旧，对当前 Emby 的能力判断、认证和错误处理不完整。
- 服务器实际在线时，客户端可能因为缺少 Jellyfin 专用接口而误报
  `Connection Failure`。

这个分支的目标不是简单更换名称或重新打包旧版本，而是建立一个可持续维护、
可验证、可回滚的个人适配版本，让现代 Mac 用户能够继续通过开源桌面播放器
连接自己的 Emby 媒体服务，浏览媒体库并播放视频。

## 优化方向

### 现代 macOS

- 修复启动阶段对系统字体目录的不安全写入。
- 在不关闭 Gatekeeper 或 SIP 的前提下完成本地运行与验证。
- 改善 Apple Silicon + Rosetta 2 环境下的启动和播放稳定性。
- 评估 Apple Silicon 原生构建、正式签名和苹果公证。

### 当前 Emby 兼容

- 根据服务器能力判断功能，不默认所有服务端都提供 Jellyfin 接口。
- 兼容 Emby 4.8 系列的服务器发现、用户名密码认证和会话建立。
- 修正 Quick Connect 接口缺失时的错误处理，避免误报服务器离线。
- 验证媒体库加载、海报与元数据、视频播放、音轨和字幕。

### 播放与体验

- 保留 MPV 播放内核及硬件解码能力。
- 检查现代 macOS 下的音频输出、全屏、刷新率和字幕表现。
- 清理过时配置和无效播放参数，降低启动及播放阶段的噪声错误。
- 为修复包建立可重复的构建、签名、打包和验收流程。

## 当前兼容状态

| 能力 | 环境 | 状态 |
| --- | --- | --- |
| 应用启动 | macOS 26.6 / Apple Silicon / Rosetta 2 | 源码修复与 Intel 测试包构建已通过，待真实界面验收 |
| HTTPS 服务器连接 | Emby 4.8.10 | 已验证服务器发现接口返回正常 |
| Quick Connect 能力判断 | Emby 4.8.10 | 已实现缺失接口兼容并通过自动测试，待真实界面验收 |
| 用户名密码登录 | Emby 4.8.10 | 连接来源已修复，待真实账号验收 |
| 媒体库浏览 | Emby 4.8.10 | 待登录修复后验证 |
| 视频、音轨与字幕播放 | Emby 4.8.10 | 待验证 |
| Apple Silicon 原生运行 | macOS | 尚未支持，目前使用 Rosetta 2 |
| 开发者 ID 签名与苹果公证 | macOS | 尚未支持 |

详细问题、范围和验收标准见
[Emby 兼容修复说明](docs/emby-compatibility-repair.md)。

## 近期路线

1. 完成 `1.8.0-dev` Intel 测试包的真实启动与 Emby 登录验收。
2. 完成媒体库、播放、音轨、字幕和断线恢复验收。
3. 根据真实验收结果补齐认证与播放兼容问题。
4. 建立 Developer ID 签名、公证和 DMG 发布流程。
5. 为后续版本建立自动更新与回滚策略。
6. 评估更新 Qt、内置网页客户端和 Apple Silicon 原生构建的成本。

## 开发与构建

当前上游构建体系基于 Qt 5.15.2、Qt WebEngine、CMake/Ninja 和 MPV。
历史构建方式可参考
[Terminus-Media 上游仓库](https://github.com/Terminus-Media/jellyfin-media-player)
及项目内的 GitHub Actions 配置。

第一阶段会优先建立当前 macOS 环境可重复执行的修复和验收流程，再决定是否
整体升级 Qt 与网页客户端，避免在没有完成核心登录、播放验证前扩大改造范围。

本地调试信息默认位于：

```text
~/Library/Logs/Terminus Player/
~/Library/Application Support/Terminus Player/
```

服务器地址、用户名、密码、访问令牌、Cookie 和运行日志不得提交到 Git。

## 项目关系

本项目的来源关系如下：

1. 本个人维护分支基于
   [Terminus-Media/jellyfin-media-player](https://github.com/Terminus-Media/jellyfin-media-player)。
2. Terminus Media Player 基于
   [Jellyfin Media Player / jellyfin-desktop-qt](https://github.com/jellyfin-archive/jellyfin-desktop-qt)。
3. Jellyfin Media Player 的桌面播放器架构源自
   [Plex Media Player](https://github.com/plexinc/plex-media-player)。

这是个人维护和兼容性研究项目，不代表 Terminus-Media、Jellyfin 或 Emby
官方立场，也不是这些项目的官方客户端。Emby、Jellyfin、Plex 及其商标归
各自权利人所有。

## 贡献原则

- 问题报告需要说明 macOS 版本、Mac 架构、服务器类型和服务器版本。
- 不要在 Issue、日志或截图中公开服务器地址、账号、令牌或私人媒体信息。
- 修复应附带可复现问题、验证路径和未覆盖范围。
- 优先解决真实连接和播放问题，不为形式完整引入无关重构。

## 许可证

本项目沿用上游的 GNU General Public License v2.0。详情见
[LICENSE](LICENSE)。

对上游项目的修改、再发布和衍生版本应继续遵守原项目及其依赖的许可证与
署名要求。
