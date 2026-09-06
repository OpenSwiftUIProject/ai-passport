简体中文 · [English](README.md)

# AI Passport — OpenSwiftUI Embedded 展示

`embed/folotoy` 分支为 FoloToy ESP32-C3 编译 OpenSwiftUI Embedded 展示与输入配置。
修改 `main/swift/ContentView.swift` 即可描述画面：

```swift
import OpenSwiftUI

struct ContentView: View {
    @State private var useBlue = false
    @State private var showImage = true

    var body: some View {
        VStack(spacing: 12) {
            if showImage {
                Image("spark").resizable().frame(width: 80, height: 80)
            }
            Text("Hello, OpenSwiftUI!")
            HStack(spacing: 8) {
                Color.red.frame(width: 32, height: 8)
                Color.blue.frame(width: 32, height: 8)
            }
        }
        .padding(12)
        .background(useBlue ? Color.blue : Color.black)
        .onPhyicButton(.up) { useBlue = false }
        .onPhyicButton(.down) { useBlue = true }
        .onPhyicButton(.ok) { showImage.toggle() }
    }
}
```

框架单独编译为 `OpenSwiftUI.swiftmodule` 与 `libOpenSwiftUI.a`，再由固件 import
和链接。共用 View 协议、builder、空 View 与条件 View，使用显式 Embedded 配置；
平台依赖较重的图元提供小型 Embedded 实现。这是**有界的 Embedded 子集**，不是桌面版
AttributeGraph/RenderBox 渲染器，也不是完整兼容 SwiftUI 的运行时。

设备开机进入 ContentView。UP/DOWN 循环切换面板配色，OK 切换图片显示；
单击 closure 更新根视图 `@State` 后重新测量布局并刷新场景。长按 OK 返回硬件菜单，
重新进入 `OpenSwiftUI` 会创建初始状态。原计数模型及适配层保留作回归测试。
场景周围保留 FoloToy 天空、草地、标题及启动电量快照。

## Workspace 与构建

默认使用同级目录结构：

```text
FoloToy/
  ai-passport/                 固件，embed/folotoy 分支
  framework/                   分支 workspace 容器
    OpenSwiftUI/               框架，embed/folotoy 分支
    OpenAttributeGraph/        同级源码 worktree，当前不链接
    ...
    build/riscv32/             独立框架编译产物
  toolchains/esp-idf-v5.5.3/
  toolchains/espressif/
  work/                       日志、预览与部署暂存，不在源码仓库中
```

workspace 使用 OpenSwiftUI-Mono 的脚本创建：

```bash
Scripts/setup.sh --worktree Repos embed/folotoy /absolute/path/FoloToy/framework
```

从固件仓库运行：

```bash
tools/with-env.sh ./tools/validate.sh             # 主机测试 + 隔离 C3 门禁
tools/with-env.sh idf.py build                    # 增量开发
tools/with-env.sh idf.py size
```

选中的框架源码变化时会自动重新编译独立 OpenSwiftUI 模块。
框架中的 `Embedded/sources.txt` 定义裁剪清单，`Embedded/idf.cmake` 接入 IDF。
首次构建前可设置 `OPENSWIFTUI_SOURCE_DIR` 指向另一份 worktree；已有 CMake 缓存
使用 `idf.py -DOPENSWIFTUI_SOURCE_DIR=... build` 修改。

工具链：**ESP-IDF 5.5.3**、带 RISC-V Embedded 库的 **Swift 6.3.1 RELEASE**、
CMake 3.29+、Ninja 和 Python 3.12。`tools/with-env.sh` 只激活当前命令，默认选择
同级工具链目录及已安装的 Swift 6.3.1，可通过 `IDF_PATH`、`IDF_TOOLS_PATH`、
`SWIFT_TOOLCHAIN` 覆盖。本机 Xcode 自带 Swift 不包含所需 RISC-V 库。
详见[最初工具链与部署记录](docs/development/engineering/embedded-swift-validation.zh_CN.md)。

## 已支持的展示子集

| API | 当前行为 |
| --- | --- |
| `struct ContentView: View`、`some View` | 静态特化的 body 遍历 |
| `@ViewBuilder`、嵌套自定义 View | 顺序组合、空块、`if`、`if/else` |
| `Color(red:green:blue:opacity:)`、常量颜色 | 固定 sRGB 填充，支持 alpha |
| `Image("spark")`、`.resizable()` | 默认采用资源固有尺寸；显式 resizable 后响应尺寸提议，以最近邻缩放 |
| `Text("literal")`、`.foregroundStyle(Color)` | 使用真实 Montserrat 14 字体测量，并按宽度换行；没有动态本地化或中文字库 |
| `VStack`、`HStack`、`VStackLayout`、`HStackLayout` | 测量与放置布局、显式间距、边缘／居中对齐及弹性空间分配 |
| `onPhyicButton(.up/.down/.ok) { ... }` | 同步单击 closure；第一个匹配的处理器消费事件 |
| `@State`、`EmbeddedViewHost { ContentView() }` | 保留根视图状态，写入后使宿主失效并经现有布局重绘 |
| `RootGeometry` | 屏幕尺寸、内容边距、根视图尺寸提议与居中 |
| `Layout` | 支持带类型缓存的泛型自定义布局，分别执行尺寸测量与放置 |
| `ZStack`、`.frame`、`.padding`、`.background`、`.offset` | 参与测量的叠放与修饰器；offset 仅改变绘制位置，不改变向父级报告的空间 |

`PassportContent` 从 C 获取 LVGL 实际屏幕尺寸与主题边距，先配置 `RootGeometry`。
240x320 面板扣除顶部 66、底部 30、左右各 12 后，目前提供 216x224 内容区域，
Swift 不再写死内容尺寸。渲染器先提出空间建议、测量子视图，再定位；文字测量与
标签绘制使用相同字体和换行规则。示例 ContentView 使用堆叠布局，没有 offset。

Embedded 的 `Layout` 使用整数像素几何和按索引访问的泛型子视图。测量／放置流程
及弹性探测遵循框架布局模型，但没有链接桌面版 AttributeGraph `RootGeometry`
规则或完整 `StackLayout` 引擎。默认间距固定为 8 像素；尚不包含基线／RTL 对齐、
layoutPriority、Spacer、完整图运行时的 State/Observation、动画、SF Symbols 或运行时
图片解码。未实现的 API 在编译时不可用。

`onPhyicButton` 保留本次指定的拼写。外层匹配的修饰器优先；否则容器按声明顺序
遍历子视图，找到第一个处理器就停止。不存在的条件分支不接收事件。这个板级输入
修饰器不要求焦点或命中测试。PRESS、DOUBLE 和 LONG 不触发页面的单击 closure。

根视图必须在 `EmbeddedViewHost` 的 builder 内构造，宿主保留到页面退出。
本配置支持保留的根视图及其存储子视图中的 `@State`；在 `body` 求值期间新构造的
有状态子视图尚不支持，会明确失败，不会静默重置。没有 `$state` Binding 投影、
自动结构身份、Observation 或异步状态调度。状态、action、渲染都必须在串行 UI
上下文执行。固件在 action 后仅当宿主失效时重绘。

BSP 回调无等待地入队，由应用生命周期的 LVGL timer 每 16 ms 最多消费 16 个事件；
队列满时丢弃新事件并报告警告。页面代次会丢弃已经退出页面的队列事件。
重绘只替换场景子节点，保留标题、电量和内容容器。接收器失败时保留状态供下一次
操作重试；退出时释放宿主。

C 场景接收器在现有 LVGL 锁下管理对象，最多允许 32 个绘图节点，拒绝非法坐标
及未知资源，复制有长度上限的文字，并在退出时删除场景对象。示例使用 8 个节点，隐藏图片后为 7 个。
MCU 不新增整屏 framebuffer；示例图片是 Flash 中 512 字节 RGB565 常量，
见[资源说明](assets/README.zh_CN.md)。

## 预览与测试

```bash
tools/with-env.sh ./tools/test-openswiftui.sh
tools/with-env.sh ./tools/preview-openswiftui.sh ../work/captures/openswiftui-preview.png
```

预览在电脑上编译同一个固定版本的 LVGL，使用真实 C 场景接收器、`ContentView`、
框架模块和图片资源，渲染 240x320 RGB565 像素，再经校验过的截图协议生成 PNG。
其中电量是明确的 **73% 主机测试数据**，不是设备截图。预览还检查按键过滤、配色／图片状态、500 次重绘、未知资源、节点
上限及 20 次页面生命周期与状态重置；需先经 IDF 构建解析出 `managed_components/lvgl__lvgl`。

主机测试覆盖真实框架与客户端模块边界、嵌套 builder、条件分支、几何和颜色，
以及固件真实 ContentView 的八个接收器失败位置。这些与实机启动、面板显示、堆
和 USB 截图检查分开记录。当前结果见
[OpenSwiftUI 验证记录](docs/development/engineering/openswiftui-embedded-validation.zh_CN.md)。

## 设备部署与 USB 截图

这台礼物此前已刷入仅应用测试固件。原始分区表**没有 Recovery 条目**，刷写前
预留 Recovery 区域为空。完整 Flash 备份私下保留在 `../work/device-backups/`，
上一个已知可用截图固件位于 `../work/deployment/screenshot-fd28679a/`。

对此设备，从经过验证的合并镜像提取应用，**只写 `0x10000` 的应用**。
保留原 bootloader、分区表、NVS、设备身份、资源及 Recovery 区域；刷写前后
重新核对设备和保留区域摘要。不要整片擦除，也不要把合并镜像从零地址整段写入
这台旧出厂布局设备。

完整门禁仅保存 `build/FoloToy-AI-Passport-full.bin`；增量构建分段的时间戳可能
不同，因此从已验证产物中提取应用用于部署。保持 3 MB 应用上限和
[BLE/Recovery 契约](docs/development/engineering/ble-recovery-compatibility.zh_CN.md)
中的保护范围。设备凭证和私有恢复链接不得进入 Git。

部署成功后，关闭其他串口监视器，执行：

```bash
tools/with-env.sh python tools/capture_screen.py --output ../work/captures/passport.png
```

工具自动寻找唯一的 Espressif USB 设备，也接受 `--port`。采用无复位 RTS/DTR
顺序，校验逐行 CRC32 与完整像素覆盖，只有收到完整帧才保存。截图暂时持有 UI
锁，复用现有 20 行 LVGL 缓冲，无法测量物理亮度或屏幕坏点。这是按需截图，
不是视频流。

远端 GitHub CI 仍需要安装 Swift 并引入对应框架修改。已知的未使用
`BUTTON_VER_*` Swift 警告及较小 Recovery 分区容量提示仍存在；经过检查的应用
属于 3 MB factory 分区。
