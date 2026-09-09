简体中文 · [English](README.md)

# OpenSwiftUI AI Passport Playground

在浏览器编写 `struct ContentView: View`，预览真实 OpenSwiftUI Embedded 布局、
State 和 Passport LVGL 绘制。此工具属于 https://github.com/OpenSwiftUIProject/ai-passport，
直接复用当前 checkout 的 C/Swift 桥接，WASM 预览不依赖 QEMU Simulator 仓库。

预期在线入口为 https://openswiftuiproject.github.io/ai-passport/。
访问者可直接操作预编译示例，也可连接自己的本地编译器。

## 从零启动

先安装 Swift 6.3.1 RELEASE、Python 3.9+、Git、CMake、Ninja 和含 npm 的 Node.js 20+。
已验证 macOS arm64；Linux 和 WSL 尚未验证。然后执行：

```sh
git clone --branch main --single-branch \
  https://github.com/OpenSwiftUIProject/ai-passport.git ai-passport
cd ai-passport
./tools/playground/start-compiler.sh \
  --allow-origin https://openswiftuiproject.github.io
```

启动器直接使用当前 Passport main checkout，准备 OpenSwiftUI 和 LVGL，
在 `http://127.0.0.1:4191/compile` 启动编译服务，不会另行克隆固定的 Passport revision。
独立的[安装 skill](https://github.com/OpenSwiftUIProject/ai-passport/tree/main/skills/ai-passport-local-compiler)
也可以在没有 checkout 时使用，网页提供其 ZIP 下载。
环境、路径覆盖和连接排错见[本地安装说明](LOCAL_COMPILER.zh_CN.md)。

## 预览行为

点击 Connect 后启用编辑。Auto preview 在停止输入 650 毫秒后编译，Build & Run
立即编译。每次成功构建会启动新的 WASM Worker 并重置 State，普通 UP/DOWN/OK
保留运行中 View 的 State。编译失败保留上一份预览，Disconnect 后仍可交互。
编辑仅保留在当前标签页，刷新后丢弃。

默认示例支持 Color、字面量 Text、`Image("spark")`、测量布局的 Stack、frame、
padding、background 和根 State/按键 closure。RootGeometry 为 240 × 320，
inset 上 66、左右 12、下 30。C 桥接提供与固件相同的位图、字体和 LVGL 场景，
电量使用固定 73% fixture。未实现完整桌面 SwiftUI、Foundation、SF Symbols、
任意资源上传或浏览器烧录固件。

## 构建与验证

从仓库根目录执行：

```sh
./tools/playground/start-compiler.sh --export-pages build/pages/ai-passport
python3 -m unittest discover -s tools/playground -p 'test_*.py'
node --test tools/playground/compiler-config.test.mjs
cd tools/playground
node smoke.mjs
node verify-static.mjs ../../build/pages/ai-passport
```

导出目录必须为空。共享构建缓存放在 `build/playground/`，WASI C 库放在仓库旁的
`toolchains/`。脚本下载 [OpenSwiftUI](https://github.com/OpenSwiftUIProject/OpenSwiftUI)
的 `e35b91a31789c3c277e9a784b9f9b86e5eb708f5` 和 [LVGL](https://github.com/lvgl/lvgl)
9.5.0 的 `85aa60d18b3d5e5588d7b247abf90198f07c8a63`。不需要 OAG 或 ESP-IDF。
API 与构建边界见[本地安装说明](LOCAL_COMPILER.zh_CN.md)。

WASM smoke 测试执行 1,000 次 State/按键更新并导出三帧；静态验证检查文件 hash
及这三帧。需要和独立生成的原生 LVGL FPS1 fixture 比较时，在 smoke 后执行
`python3 compare-host.py /path/to/frame.fps1`。已有初始/DOWN/隐藏图片 fixture
每帧 76,800 个像素全部相同。主机速度和内存不代表实机性能，网页测试不涉及设备。

GitHub Pages 工作流和发布目录见[部署说明](DEPLOYMENT.zh_CN.md)。

## 固件与 Simulator

成功预览后点击 Build firmware，下载匹配源码的 full.bin 或 Send to Simulator，再点 Open Simulator。固件需要额外安装 ESP-IDF 5.5.3；Simulator 需要支持导入 API 并允许当前页面 origin。完整步骤与限制见 [本地说明](LOCAL_COMPILER.zh_CN.md)。此流程不烧录实机。
