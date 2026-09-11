[English](DEPLOYMENT.md) · 简体中文

# GitHub Pages 部署

目标仓库是 [OpenSwiftUIProject/ai-passport](https://github.com/OpenSwiftUIProject/ai-passport)，
公开入口 https://openswiftuiproject.github.io/ai-passport/。
Pages 工作流生成静态前端、示例 WASM 和独立 setup skill。访客直接玩示例；任意 Swift
编辑与固件构建使用自己的本地编译器。[Simulator Pages](https://openswiftuiproject.github.io/FoloToy-Passport-Simulator/) 在浏览器中运行 QEMU/WASM；可选 Node 版补充联网能力。网站作者无需保持电脑在线。

## 本地构建与预览

从上述完整仓库 URL clone **main**，按[本地编译器说明](LOCAL_COMPILER.zh_CN.md)准备依赖：

```sh
./tools/playground/start-compiler.sh --export-pages build/pages/ai-passport
cd tools/playground
node smoke.mjs
node verify-static.mjs ../../build/pages/ai-passport
cd ../..
python3 -m http.server 4192 --bind 127.0.0.1 --directory build/pages
```

打开 `http://127.0.0.1:4192/ai-passport/`。页面从实际 URL 推导允许来源。
所有资源使用相对 URL，包括 worker 和可下载的 `skills/ai-passport-local-compiler.zip`。
旁边的 `build/pages/ai-passport.zip` 为静态站点包。导出需要空目录，包含一致的
ContentView/WASM、本地编辑器依赖、许可、递归 SHA256SUMS 和 `.nojekyll`。
示例启动前检查源码与 WASM 的哈希。

## 发布已确认的改动

仓库的 `.github/workflows/playground-pages.yml` 在相关 PR 和 main push 上构建、测试，
只有 `OpenSwiftUIProject/ai-passport` 的 main 才通过独立部署 job 发布。
在 Settings → Pages 选择 **GitHub Actions**，合入已确认 PR，以部署 job 返回的 URL 为
实际入口，也支持手动触发。本地准备不会修改 Pages 设置或发布。

构建使用 macOS、Swift 6.3.1、Node 22、CMake/Ninja 和固定版本依赖，不安装 ESP-IDF，
也不预打包设备固件；固件由访客本机按需生成。已于 2026-09-10 在 macOS 的 Codex
内嵌浏览器验证公开 Pages 部署及 HTTPS 到回环服务的完整流程：编辑 Swift、预览、
构建固件和导入 Simulator。其他浏览器的权限流程尚未验证；这些检查没有烧录实机。

## 社交分享

初始 HTML 包含 Open Graph 和 X 大图卡片元数据，使用规范公开 URL 与绝对 HTTPS 图片
地址，抓取器不需要运行编辑器。导出时将 `assets/images/openswiftui-playground-social.png`
复制为 `social-card.png`，纳入校验和与网站 ZIP；本地编译器也提供该图片路径。
可编辑卡片源文件和预览来源见[资源记录](https://github.com/OpenSwiftUIProject/ai-passport/blob/main/assets/README.zh_CN.md)。
发布后需检查公开图片 URL 及 X 上的卡片，本地验证无法确定 X 何时刷新已缓存的链接预览。

## 编译器与 Simulator 协议

网页编译器默认 `http://127.0.0.1:4191/compile`，必须点击 Connect；`?compiler=...`
和历史 URL 只预填。允许 HTTPS 服务或 HTTP 回环地址，拒绝公开 HTTP、URL 凭据、
查询和 fragment。本地服务用于可信开发，不面向互联网公共编译。

- `GET /health` 返回 `openswiftui-passport-compiler`、协议 1，以及可选 ESP-IDF 的
  `firmwareAvailable` 能力。
- `POST /compile`，JSON `{ "source": "..." }`，返回预览 WASM 或 JSON 错误。
- `POST /firmware` 使用相同 JSON，返回任务 ID 和源码 SHA-256。
- `GET /firmware/<id>` 返回 building/ready/failed 和有限长度的日志尾部。
- `GET /firmware/<id>/download` 在 ready 时返回已检查完整固件。
- 跨源静态 Simulator 的 `GET playground-config.json` 声明 `windowHandoff: true`。
  Open 点击时创建窗口并清空 opener；`window-handoff.js` 检查随机 48 位十六进制 token、
  精确 origin 与 WindowProxy，以 hello/ready/firmware/received 握手传递，校验完整镜像及
  SHA-256。不向远端服务器上传二进制；编辑、重置或断开会取消未完成的交接。
  本地预览使用与 Pages 一样的线上 Simulator URL。
- 同源 Simulator 的 `GET playground-config.json` 声明协议 1 与 `indexeddb` 传输。
  两仓库各自提供兼容的 `browser-handoff.js`。数据库为 `openswiftui-passport-handoff-v1`，
  store 为 `firmware`，键为随机 48 位十六进制 `id`。记录包含 `version: 1`、ArrayBuffer
  `bytes`、`sha256`、`sourceSha256`、目标基础 URL 与毫秒 `expiresAt`；读取时检查
  大小、镜像头、期限、目的地和 SHA-256。运行链接保留项目路径
  `/FoloToy-Passport-Simulator/?playground=<id>`，仅同一浏览器配置/来源可用，不能当固件分享链接。
- 本地 Node Simulator `POST /api/playground-firmware` 接收带 `X-Firmware-SHA256` 的镜像，
  返回有时效的 `/?playground=<id>` 运行链接；需要本地上传能力及显式允许的 Playground 来源。

完整 clone 命令、ESP-IDF / Simulator 安装、浏览器权限和资源限制见
[本地说明](LOCAL_COMPILER.zh_CN.md)。
