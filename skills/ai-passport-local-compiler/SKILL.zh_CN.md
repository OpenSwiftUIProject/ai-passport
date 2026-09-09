---
name: ai-passport-local-compiler
description: 从零准备访问者本机的 Swift 编译器，克隆公开 fork、安装构建依赖并连接 AI Passport OpenSwiftUI 网页 Playground。
---
简体中文 · [English](SKILL.md)

# AI Passport 本地 Swift 编译器

在访问者自己的电脑上准备编译器并连接网页 Playground。本 skill 可单独下载，
不能假定已经有项目 checkout、Simulator 仓库、Swift 工具链或旁边的 workspace。

- 项目：https://github.com/OpenSwiftUIProject/ai-passport
- 克隆地址：`https://github.com/OpenSwiftUIProject/ai-passport.git`
- 项目分支：**main**，不要固定 Passport revision。
- 预期 Pages 入口：https://openswiftuiproject.github.io/ai-passport/
- 默认编译器 URL：`http://127.0.0.1:4191/compile`。

## 准备电脑

先检查系统、架构和现有工具。已验证 macOS arm64；需要 **Swift 6.3.1 RELEASE**，
包含 Embedded wasm32 库及 clang、clang++、wasm-ld、llvm-ar、llvm-ranlib，
以及 Python 3.9+、Git、CMake、Ninja、Node.js 20+ 和 npm。快速 WASM 预览不需要固件工具或 ESP-IDF。

macOS 优先复用 Xcode Command Line Tools 和 Homebrew。缺少构建工具时执行
`brew install cmake ninja node python git`。如果缺少 Xcode 工具或 Homebrew，按需
使用 `xcode-select --install` 和 https://brew.sh 官方安装器。
从 https://www.swift.org/install/ 官方下载页安装 Swift 6.3.1；已安装 Swiftly 时
也可执行 `swiftly install 6.3.1`。保留已有 Swift 版本，不修改 shell 启动文件或全局
工具链选择。安装器需要交互式管理员操作时，让用户完成该步后继续。

用选定工具链的 `swiftc --version` 确认版本。启动器优先识别
`~/Library/Developer/Toolchains/swift-6.3.1-RELEASE.xctoolchain/usr/bin/swiftc`；
其他路径通过单次命令的 `SWIFTC` 指定完整编译器路径。
Linux 和 Windows/WSL 尚未验证，要区分实验尝试和已验证的 macOS 路径。

## 克隆并启动

从用户请求或当前页面取得 Playground URL。把实际 **origin**（协议、主机和可选端口，
不含路径）传给 `--allow-origin`。预期 Pages 对应 `https://openswiftuiproject.github.io`。
本地预览则使用那个页面的实际 origin，不固定开发端口。

在解压后的 skill 目录执行所附脚本，不需要先 clone：

```sh
bash scripts/bootstrap.sh \
  --allow-origin https://openswiftuiproject.github.io
```

脚本从上面的完整 URL 克隆 `main` 到 `~/Developer/ai-passport`。用
`--directory /chosen/path/ai-passport` 改变位置。已有匹配的 main checkout 会直接复用，
不会修改现有工作；其他目录或分支会被拒绝，不要强行覆盖。已有 checkout 过旧时，
先保留改动再更新，或换一个新目录。

对空目录手动执行的等价命令如下；仅下载 SKILL.md、没有辅助脚本时也可使用：

```sh
git clone --branch main --single-branch \
  https://github.com/OpenSwiftUIProject/ai-passport.git ai-passport
cd ai-passport
./tools/playground/start-compiler.sh \
  --allow-origin https://openswiftuiproject.github.io
```

启动器使用**当前 Passport checkout**，下载匹配的 OpenSwiftUI 与 LVGL，校验
WASI SDK 34 C 库、安装锁定的编辑器依赖并缓存共享构建。不会另外 checkout 固定的
Passport revision。保持终端运行，Ctrl-C 停止。需要换端口时用 `--port 4201`，
同时把网页 Compiler URL 改为对应端口。

## 连接并验证

打开用户指定的 Playground，填写编译器 URL 并点击 Connect。浏览器询问本地/回环
网络权限时允许访问。`?compiler=...` 仅预填地址，不会自动发送源码。
确认页面显示 Connected，修改 ContentView 中一段字面量 Text，确认构建后显示新文字。
操作默认示例的 UP/DOWN/OK，并确认 Disconnect 后预览仍可交互。

连接失败时检查 `http://127.0.0.1:4191/health`、终端端口和允许的 origin。
HTTPS 页面访问本机受各浏览器权限策略影响，可改用 `http://127.0.0.1:4191/`
本地编辑器。不要关闭浏览器安全设置，不要绑定公网或添加隧道。
编译器以用户身份运行，供可信本地开发使用，不是公网编译沙箱。

报告 checkout 路径、Swift 版本、编译器 URL、允许的 origin、实际构建/浏览器验证
结果及剩余兼容性问题。本流程不涉及实机烧录、Pages 发布、commit 或 push。

## 可选：固件与 Simulator

用户需要下载或模拟运行固件时，在 clone 后读取 `tools/playground/LOCAL_COMPILER.zh_CN.md` 的完整 ESP-IDF 5.5.3 和 Simulator 安装命令。检查 Swift Embedded RISC-V 库、复用已有 IDF_PATH / IDF_TOOLS_PATH，否则按说明安装到同级 toolchains。重启编译器并重新 Connect。成功预览后 Build firmware，核对源码匹配，再 Download full.bin 或 Send to Simulator。Simulator 单独 clone 完整 URL `https://github.com/OpenSwiftUIProject/FoloToy-Passport-Simulator.git`，npm ci 后 `npm start -- --playground-origin <当前网页实际 origin>`；需要包含导入 API 的版本。使用已包含此集成的 OpenSwiftUIProject fork main；旧版仍可通过本地文件选择器加载下载的固件。不要将 WASM 当作设备固件，也不要自动烧录。
