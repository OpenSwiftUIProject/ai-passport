[English](DEPLOYMENT.md) · 简体中文

# GitHub Pages 部署

目标仓库是 [OpenSwiftUIProject/ai-passport](https://github.com/OpenSwiftUIProject/ai-passport)，
预期入口 https://openswiftuiproject.github.io/ai-passport/。
Pages 工作流生成静态前端、示例 WASM 和独立 setup skill。访客直接玩示例；任意 Swift
编辑与固件构建使用自己的本地编译器。Simulator 单独运行 Node/QEMU，网站作者无需保持电脑在线。

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
也不预打包设备固件；固件由访客本机按需生成。此本地改动尚未验证公开 Pages 部署和
公开 HTTPS 页面到回环服务的权限行为。

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
- Simulator `POST /api/playground-firmware` 接收带 `X-Firmware-SHA256` 的镜像，
  返回有时效的 `/?playground=<id>` 运行链接；需要本地上传能力及显式允许的 Playground 来源。

完整 clone 命令、ESP-IDF / Simulator 安装、浏览器权限和资源限制见
[本地说明](LOCAL_COMPILER.zh_CN.md)。
