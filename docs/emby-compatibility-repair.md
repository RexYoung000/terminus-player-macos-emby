# Terminus Player Emby 兼容修复

## 项目目标

让 Terminus Player 在当前 Apple Silicon Mac 和 Emby 4.8.10 上完成以下流程：

1. 正常启动，不再尝试向 `/usr/local/etc/fonts` 写入配置。
2. 正确识别并连接 Emby 服务器。
3. 不因 Jellyfin Quick Connect 接口缺失而误报服务器离线。
4. 使用真实账号完成登录。
5. 加载媒体库并完成至少一次视频播放。

## 当前状态

- 原始项目源码已从 `Terminus-Media/jellyfin-media-player` 克隆。
- 个人维护仓库以 `main` 作为默认分支；第三方上游保留为 `upstream`，
  便于后续审查和同步来源变化。
- 已通过成品应用二进制补丁解决 macOS 启动崩溃，修复版保存在
  `artifacts/TerminusPlayer-1.7.1-macos-startup-fixed.dmg`。
- 服务器、HTTPS、DNS 和 443 端口均已确认正常。
- 当前阻塞发生在客户端兼容层：
  - 内置 Jellyfin Web 版本约为 10.7.6。
  - 目标服务器为 Emby 4.8.10。
  - 客户端请求 `/QuickConnect/Status` 时收到 `404`。
  - 旧请求逻辑随后误报 `Connection Failure`。

## 第一阶段范围

第一阶段只保证当前 Emby 4.8.10 的核心使用链路，不在本阶段升级整个
Qt、MPV 或 Web 客户端技术栈。

本阶段实施决定：

- 将 macOS 启动修复落实到可维护的源码或可重复构建流程。
- 内置网页客户端由仅监听 `127.0.0.1:43110` 的本机服务提供，不再以
  `file://` 页面直接发起跨域认证请求。固定来源用于保留服务器记录和登录
  会话；服务只读取应用内的网页资源，不对局域网开放。
- 根据服务器能力决定是否启用 Quick Connect；`/QuickConnect/Status`
  返回 `404` 或 `405` 时按“不支持”处理，不能触发服务器离线或自动重连。
- 修复 Emby 用户名密码认证请求与失败处理。
- 将已经失效的 `af-defaults=lavrresample` 和 `af del` 替换为当前 MPV
  支持的音频属性与过滤器命令。
- 重新签名、打包并执行真实界面验收。

第一阶段仍以 Intel `x86_64` + Rosetta 2 为交付基线。Apple Silicon
原生构建、Qt 整体升级和正式苹果公证继续作为后续里程碑，避免与登录和播放
故障混在同一次迁移中。

暂不处理：

- Apple Silicon 原生编译。
- 正式开发者 ID 签名与苹果公证。
- 对所有历史 Emby/Jellyfin 版本提供兼容承诺。

## 验收

- macOS 26.6 Apple Silicon + Rosetta 2 下可启动。
- 服务器连接页能进入账号登录页。
- 输入有效账号后不出现通用 `Connection Failure`。
- 媒体库首页可加载。
- 选择媒体后能开始播放，音频、视频和字幕至少完成一次基础验证。
- 运行日志不再出现 `Request scheme 'file' is unsupported`、
  `af-defaults is deprecated` 或 `lavrresample doesn't exist`。
- 测试后 `/usr/local/etc/fonts` 仍未被应用创建或修改。

## 安全与隐私

- 不向 Git 提交服务器地址、用户名、密码、访问令牌、Cookie 或运行日志。
- 本地 DMG 和调试数据分别放在 `artifacts/` 与 `repair-data/`，默认不提交。
- 不关闭 Gatekeeper 或 SIP，不使用管理员权限运行播放器。
