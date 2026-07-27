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
- 已删除导致 macOS 启动崩溃的系统字体目录写入逻辑，并通过 Intel macOS
  自动构建、依赖打包和临时签名验证。
- 内置网页已改为仅绑定 Loopback 的固定 HTTP 来源，不再从 `file://`
  页面发起认证请求。
- Emby Quick Connect 缺失接口的兼容层已通过自动测试。
- 便携 ZIP 测试包已经能由 GitHub Actions 重复生成。
- 服务器、HTTPS、DNS 和 443 端口均已确认正常。
- 首个 CI 便携包在 Apple Silicon + Rosetta 2 上通过架构、依赖路径和
  临时签名检查，但真实启动暴露 `Failed loading SDL3 library`。这是 MPV
  对运行时加载库的隐式依赖，未出现在普通 Mach-O 依赖扫描中。
- 当前先补齐 SDL3 运行时依赖及启动冒烟测试，再继续 Emby 4.8.10 的
  服务器连接页、账号登录、媒体库和播放链路验收。
- 本机 HTTP 来源恢复 Service Worker 后，macOS 更新插件直接跨域请求 GitHub
  会被 CORS 拒绝。更新检查应改由 Qt 原生网络层完成；无法提供 SSL 的平台
  静默跳过，不能在网页层绕过同源策略。
- 真实服务器地址已成功进入登录页，证明 TLS、`/System/Info/Public` 和
  基础同源请求正常。登录页随后请求 `/QuickConnect/Enabled`，Emby 返回
  `404`，旧客户端触发自动重连并将后续认证失败统一显示为
  `Connection Failure`。
- 兼容层需同时按响应类型处理两个缺失接口：`/QuickConnect/Enabled`
  返回布尔 `false`，`/QuickConnect/Status` 返回字符串 `Unavailable`。
  用户名密码已仅在取消“Remember Me”的状态下用于一次真实认证测试。

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
- 将个人维护开发版标记为 `1.8.0-dev`，与上游旧版 `1.6.1` 和历史
  macOS 成品 `1.7.1` 区分；macOS Bundle 使用符合系统格式要求的
  `1.8.0`。
- 更新检查只访问 `RexYoung000/terminus-player-macos-emby` 的 Release，
  不再把 Jellyfin 官方版本误报为本项目更新。
- 日志中的查询参数、JSON 字段及 Bearer、MediaBrowser、`X-Emby-Token`
  认证头必须整段脱敏，不能只遮住固定长度后保留剩余内容。
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
- 测试包显示 `1.8.0-dev`，且更新入口不跳转到 Jellyfin 官方仓库。
- 自动测试确认不同长度和不同格式的认证令牌不会保留在日志文本中。
- 测试后 `/usr/local/etc/fonts` 仍未被应用创建或修改。

## 安全与隐私

- 不向 Git 提交服务器地址、用户名、密码、访问令牌、Cookie 或运行日志。
- 本地 DMG 和调试数据分别放在 `artifacts/` 与 `repair-data/`，默认不提交。
- 不关闭 Gatekeeper 或 SIP，不使用管理员权限运行播放器。
