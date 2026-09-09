[English](LOCAL_COMPILER.md) · 简体中文

# 本地 Swift 编译器

托管页面可直接运行预编译的 WASM 示例。编辑 Swift 时，在自己的电脑上运行编译器，
然后点击 **Connect**；网站作者不需要保持电脑在线，也不要求你事先 clone 项目。

## 从零启动

安装 Swift 6.3.1 RELEASE（含 Embedded wasm32 库与 LLVM 工具）、Python 3.9+、
Git、CMake、Ninja、Node.js 20+ 和 npm。已验证 macOS arm64，Linux / WSL 尚未验证。
使用 [Swift 官方安装器](https://www.swift.org/install/)；macOS 其他工具可通过
`brew install cmake ninja node python git` 安装。

```sh
git clone --branch main --single-branch https://github.com/OpenSwiftUIProject/ai-passport.git
cd ai-passport
./tools/playground/start-compiler.sh --allow-origin https://openswiftuiproject.github.io
```

网页通过 `location.origin` 动态生成命令，使用当前页面的实际来源，包含本地预览的端口。
参数只包含协议、主机、可选端口，不包含 `/ai-passport/` 路径。
多个可信来源可重复传 `--allow-origin`；同一来源的所有页面共享此权限。

也可以把网页提供的 **AI Passport local compiler skill** ZIP 交给 AI 助手。
[Skill 源码](https://github.com/OpenSwiftUIProject/ai-passport/tree/main/skills/ai-passport-local-compiler)
包含独立 clone 脚本和完整说明，两种方式都使用公开 fork 的 **main**，不固定 Passport
revision，也不需要 Simulator 仓库。

启动器在 `build/playground/` 准备固定版本的 OpenSwiftUI / LVGL，检查同级
`toolchains/` 下的 WASI SDK 34，安装锁定的编辑器依赖并缓存编译结果。首次需要联网。
Passport 直接使用当前 checkout，不再额外下载一份。快速 WASM 预览不需要 ESP-IDF 或 OAG。

优先选择 macOS 上的 Swift 6.3.1 release toolchain，否则使用 PATH。可设置
`SWIFTC=/完整路径/usr/bin/swiftc`。端口占用时使用 `--port 4201`；`--prepare-only`
只准备环境；`--cache-dir /另一路径` 使用独立依赖与对象缓存。
`OPENSWIFTUI_SOURCE_DIR` / `LVGL_SOURCE_DIR` 显式选择已有开发源码；默认版本见
[README](README.zh_CN.md)。

保持终端运行，网页 Compiler URL 默认 `http://127.0.0.1:4191/compile`。
点击 Connect，浏览器询问时允许本地网络访问。历史 URL 或 `?compiler=...` 仅预填，
不会自动连接。修改 Swift 后自动预览，或点击 Build & Run。Disconnect 保留可交互预览。
编辑内容仅在当前标签页中，刷新会丢失，不会写入仓库。

## 构建固件并交给 Simulator

成功预览后，**Build firmware** 将同一份 ContentView 编译为设备固件。
此可选步骤需要 ESP-IDF **5.5.3** 和 Swift Embedded RISC-V 库。
从 Passport checkout 为新环境安装：

```sh
mkdir -p ../toolchains
export IDF_PATH="$(cd ../toolchains && pwd)/esp-idf-v5.5.3"
export IDF_TOOLS_PATH="$(cd ../toolchains && pwd)/espressif"
git clone --branch v5.5.3 --recursive https://github.com/espressif/esp-idf.git "$IDF_PATH"
"$IDF_PATH/install.sh" esp32c3
./tools/playground/start-compiler.sh --allow-origin https://openswiftuiproject.github.io
```

已有兼容安装时直接复用，通过 `IDF_PATH` / `IDF_TOOLS_PATH` 传给启动器，勿覆盖目录。
系统前置依赖和故障排查见仓库的
[环境说明](https://github.com/OpenSwiftUIProject/ai-passport/blob/main/docs/development/engineering/environment-setup.zh_CN.md)。

首次固件构建较慢，后续复用独立 C/Swift 缓存；页面可展开构建日志。服务会检查完整镜像
与 bootloader、应用、分区表及保护区规则一致。**Download full.bin** 下载通过检查的
镜像，并校验 SHA-256。它启动当前 ContentView，完整镜像对应 **0x0** 偏移。
工具不直接烧录，也不输出仅应用镜像；其他实机安装流程应保留设备身份与 Recovery。

使用包含 Playground 导入 API 的 Simulator 在 QEMU 中运行。另开目录与终端：

```sh
git clone https://github.com/OpenSwiftUIProject/FoloToy-Passport-Simulator.git
cd FoloToy-Passport-Simulator
npm ci
npm start -- --playground-origin https://openswiftuiproject.github.io
```

此命令也要用当前页面的实际 origin。Simulator 默认 `http://127.0.0.1:4190/`。
输入 URL，点击 **Send to Simulator**，再点 **Open Simulator** 即可运行，显式链接可避开
弹窗拦截。传递的是同一份固件字节并校验 SHA-256，不会发布到社区。
Simulator 最多在内存保留 3 份镜像、10 分钟有效，跨源隔离保持开启。
必须同时启用本地上传能力与显式 `--playground-origin`，默认仅社区部署会拒绝此 API。
OpenSwiftUIProject fork 的 main 包含此集成；旧版仍可通过本地固件文件选择器加载下载的 full.bin。

修改代码后，下载和发送按钮会失效，直到当前源码预览成功且有匹配固件。
预览中的 State 不会传过去，Simulator 从初始状态启动。WASM 与固件属于不同编译目标，
运行行为可能不同。

## 故障排查与限制

- 拒绝连接：确认终端仍运行且端口正确。
- 来源错误：localhost、127.0.0.1 和不同端口是不同来源。
- 浏览器拦截回环访问：允许本地网络访问，或打开 `http://127.0.0.1:4191/` 本地编辑器。
  各浏览器及内嵌 WebView 行为不同；公开 HTTPS Pages 到回环服务尚未验证。勿关闭浏览器
  安全机制或通过公共隧道暴露本地编译器。
- 缺少 ESP-IDF：安装后重启服务并重新 Connect，以刷新能力。
- Simulator 链接过期：重新发送；拒绝导入时检查版本、本地上传能力与允许来源。
- 缓存源码变动：启动器拒绝覆盖，换缓存目录或显式指定开发源码。

服务仅绑定回环地址，允许指定网页来源，以当前用户身份运行，面向可信本地开发。
JSON 请求上限 64 KiB；WASM 编译限时 30 秒；固件同时只构建一个、限时 20 分钟，
每次服务会话最多 8 份结果。Ctrl-C 停止服务及正在进行的固件构建。
结果和日志留在忽略目录 `build/playground/firmware-jobs/`，不用时可清理旧会话目录。
同一 checkout 的固件缓存共享，请只运行一个会构建固件的服务。

静态部署与 API 概览见 [部署说明](DEPLOYMENT.zh_CN.md)。
