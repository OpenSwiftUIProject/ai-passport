简体中文 · [English](README.md)

# AI Passport — OpenSwiftUI Embedded 展示

本 fork 的 `main` 分支为 FoloToy ESP32-C3 编译 OpenSwiftUI Embedded 展示与输入配置。
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

## OpenSwiftUI 2048

长按 OK 离开开机展示页，再在硬件菜单中选择 **2048**。原来的 OpenSwiftUI
展示 Demo 仍然保留。

| 输入 | 纵向模式（默认） | 横向模式 |
| --- | --- | --- |
| UP | 向上移动 | 向左移动 |
| DOWN | 向下移动 | 向右移动 |
| 短按 OK | 切换为横向 | 切换为纵向 |
| 长按 OK | 返回菜单 | 返回菜单 |

底部始终显示当前映射。切轴保留棋盘和分数，不生成新方块。UP/DOWN 按下时走一步，
OK 短按在松开时生效，无需等待双击窗口。快速连按按独立操作处理，按住方向键
不会连续走棋。电源键仍控制硬件供电。

每局从两个方块开始。有效移动后新增一个方块（90% 为 2，10% 为 4），每个方块
每步最多合并一次。无效移动不改变棋盘、分数或随机序列。达到 2048 或无路可走
时结束本局，短按 OK 以默认纵向模式重新开始。退出页面也会丢弃本局。
此版本没有持久化、撤销或滑动输入。

- 规则：`main/swift/PassportCore/Game2048.swift`，与 SwiftPM 测试共用。
- 界面：`main/swift/Game2048/Game2048View.swift`，用根视图的 `@State` 保留游戏。
- 棋盘：四行明确类型的 `VStack`／`HStack` 背景，以及放置移动方块的自定义
  `Layout`；方块边长 42、间距 4、内边距 4。OpenSwiftUI 正常测量、放置
  188x188 棋盘，没有 offset。
- 绘制：`main/openswiftui_scene.c` 提供页面专用 `RootGeometry` 边距（上 46、左右
  各 12、下 34）和共用 LVGL 接收器。分数位于开机电量左侧，操作提示位于草地上方。
- 输入：`main/demo_2048.c` 把按下／松开事件转换为现有 OpenSwiftUI 按键 closure。
  长按 OK 仍由应用菜单处理。

固件和主机预览为游戏场景使用有容量上限的 48 KB LVGL 内存池，比原展示 Demo
多占 24 KB 静态 RAM。64 位主机预览检查内存池并报告峰值；设备堆内存测量
仍需单独上板验收。

带动画的游戏需要 `embed/folotoy` 分支上的 OpenSwiftUI Embedded profile 4，
无需增加其他框架仓库。当前配置只提供静态文本，尚无动态 `ForEach`，因此有限的
方块标签和六位分数用 `StaticString` 表示，背景行及前景槽位明确声明。
参考了 [eleev/swiftui-2048](https://github.com/eleev/swiftui-2048) 与
[unixzii/SwiftUI-2048](https://github.com/unixzii/SwiftUI-2048) 的游戏模型、棋盘和
方块视图分离方式。本实现为 Embedded 环境重新编写，没有引入其应用代码、
资源、Combine 或手势处理。

## 仓库布局与构建

使用两个独立 Git clone：本固件 fork 的 `main` 分支，以及 OpenSwiftUI 的
`embed/folotoy` 分支。`framework/` 下只放 OpenSwiftUI 这一份源码仓库，
其 `.git/` 目录归该 clone 独立所有。此布局不使用关联 Git worktree，
也不需要 OpenSwiftUI-Mono 的 workspace 脚本。

此 Embedded 配置不编译或链接 OpenAttributeGraph（OAG）、OpenRenderBox、
OpenCoreGraphics、OpenObservation、Compute 或 DarwinPrivateFrameworks，
因此无需克隆其他框架仓库。

目录结构：

```text
FoloToy/
  ai-passport/                 固件独立 clone，main 分支
  framework/
    OpenSwiftUI/               独立克隆，embed/folotoy 分支
  toolchains/esp-idf-v5.5.3/
  toolchains/espressif/
  work/                       日志、预览、构建产物与部署暂存
```

首次克隆时，获取本固件 fork 的 `main` 分支与 OpenSwiftUI 的 `embed/folotoy` 分支：

```bash
mkdir -p FoloToy/framework
cd FoloToy
git clone --single-branch --branch main https://github.com/OpenSwiftUIProject/ai-passport.git ai-passport
git clone --single-branch --branch embed/folotoy https://github.com/OpenSwiftUIProject/OpenSwiftUI.git framework/OpenSwiftUI
cd ai-passport
```

### 为已有固件 checkout 配置 OpenSwiftUI

在 `ai-passport` 仓库根目录运行，将框架克隆到默认位置：

```bash
mkdir -p ../framework
git clone --single-branch --branch embed/folotoy \
    https://github.com/OpenSwiftUIProject/OpenSwiftUI.git ../framework/OpenSwiftUI
```

固件与主机脚本会自动发现 `../framework/OpenSwiftUI`，此布局无需设置环境变量覆盖。
ESP-IDF 在自己的构建目录内生成框架模块和静态库；LVGL 主机预览的构建产物位于
`../work/openswiftui-preview/`，单独构建框架时则写入 `--output` 指定的位置。

如果已在其他位置克隆了该 OpenSwiftUI 分支，可直接复用，指定其绝对路径，无需再次克隆：

```bash
export OPENSWIFTUI_SOURCE_DIR=/absolute/path/to/OpenSwiftUI
```

该 checkout 需要包含 `Embedded/sources.txt` 和 `Embedded/idf.cmake`。
固件会自动构建并链接框架，无需手动复制静态库或配置 OAG。
独立主机构建与渲染接收器的接口要求见
[OpenSwiftUI Embedded 指南](https://github.com/OpenSwiftUIProject/OpenSwiftUI/blob/embed/folotoy/Embedded/README.md)。

### 选择工具链并构建

按[环境指南](docs/development/engineering/environment-setup.zh_CN.md)安装
ESP-IDF 5.5.3 及 ESP32-C3 工具，再安装带 Embedded RISC-V 库的 Swift 6.3.1 RELEASE。
复用默认同级目录之外的已有安装时，显式指定路径：

```bash
export IDF_PATH=/absolute/path/to/esp-idf-v5.5.3
export IDF_TOOLS_PATH=/absolute/path/to/espressif-tools
export SWIFT_TOOLCHAIN=/absolute/path/to/swift-6.3.1-RELEASE.xctoolchain
```

从固件仓库运行；首次构建会解析锁定版本的 Managed Components，包含 LVGL 与
`espressif/idf_swift`：

```bash
tools/with-env.sh ./tools/test-openswiftui.sh     # 验证框架与 ContentView 接入
tools/with-env.sh ./tools/validate.sh             # 主机测试 + 隔离 C3 门禁
tools/with-env.sh idf.py build                    # 增量开发
tools/with-env.sh idf.py size
```

选中的框架源码变化时会自动重新编译独立 OpenSwiftUI 模块。
框架中的 `Embedded/sources.txt` 定义裁剪清单，`Embedded/idf.cmake` 接入 IDF。
首次构建前可设置 `OPENSWIFTUI_SOURCE_DIR` 指向另一份 clone；已有 CMake 缓存
使用 `idf.py -DOPENSWIFTUI_SOURCE_DIR=... build` 修改。
框架的 `Embedded/README.md` 还说明了独立主机构建与测试，不需要 ESP-IDF 或 LVGL。
框架构建脚本同时启用 LVGL 标志和编译器的 Embedded Swift 模式，对应
`OPENSWIFTUI_LVGL && hasFeature(Embedded)` 条件。

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
| `onPhyicButton(.up/.down/.ok) { ... }` | 平台分发的同步按键 closure；第一个匹配的处理器消费事件 |
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
layoutPriority、Spacer、完整图运行时的 State/Observation、桌面动画 API、SF Symbols 或运行时
图片解码。未实现的 API 在编译时不可用。

`onPhyicButton` 保留本次指定的拼写。外层匹配的修饰器优先；否则容器按声明顺序
遍历子视图，找到第一个处理器就停止。不存在的条件分支不接收事件。这个板级输入
修饰器不要求焦点或命中测试。展示页只转发 CLICK；2048 转发 UP/DOWN 的 PRESS
以及配对的 OK 短按 RELEASE。DOUBLE 不增加操作，长按 OK 通过菜单返回。

根视图必须在 `EmbeddedViewHost` 的 builder 内构造，宿主保留到页面退出。
本配置支持保留的根视图及其存储子视图中的 `@State`；在 `body` 求值期间新构造的
有状态子视图尚不支持，会明确失败，不会静默重置。没有 `$state` Binding 投影、
自动结构身份、Observation 或异步状态调度。状态、action、渲染都必须在串行 UI
上下文执行。固件在 action 后仅当宿主失效时重绘。

BSP 回调无等待地入队，由应用生命周期的 LVGL timer 每 16 ms 最多消费 16 个事件；
队列满时丢弃新事件并报告警告。页面代次会丢弃已经退出页面的队列事件。
重绘按绘制顺序复用兼容的场景对象，保留标题、电量和内容容器。接收器失败时保留状态供下一次
操作重试；退出时释放宿主。

C 场景接收器在现有 LVGL 锁下管理对象，展示页最多允许 32 个绘图节点，2048
最多允许 64 个；拒绝非法坐标及未知资源，复制有长度上限的文字，退出时删除
场景对象。展示示例使用 8 个节点，隐藏图片后为 7 个；包含固定空格背景及六位分数的 2048 动画满盘最多使用 58 个。
MCU 不新增整屏 framebuffer；示例图片是 Flash 中 512 字节 RGB565 常量，
见[资源说明](assets/README.zh_CN.md)。

### 2048 动画

游戏使用 `withAnimation(.linear(duration: 0.24))` 完成方块移动，随后用
160 ms 的缩放展示合并结果，缩放／淡入展示新方块。Swift 规则模型记录参与合并的两个
原方块及共同目的地，移动完成后才显示结果数字。未合并方块保留 ID，合并与
新生方块获得新 ID。自定义 `Layout` 将 16 个前景槽位放置到目标格；固定背景及
周边 UI 仍使用 VStack/HStack，没有 offset，也不使用整屏位图。

需要 OpenSwiftUI Embedded **profile 4**，包含 `withAnimation`、`.id(UInt32)`
及 `.transition(.scale.combined(with: .opacity))`。请一起重新构建框架与固件。
这仍是有限动画子集，不包含 `.animation(_:value:)`、弹簧或完整桌面 Animatable。

页面自己的 LVGL timer 按实际经过的毫秒数推进宿主，仅在需要时生成下一帧。
请求间隔为 16 ms；实际面板帧率取决于绘制和 SPI 耗时，需要实机测量。各阶段在
布局完成、生成首帧后才开始计时，避免布局和此前空闲时间跳过开头的动画帧。
渲染器复用 LVGL 对象，仅更新变化的几何、样式和标签文字。LVGL 池仍为 48 KB，
游戏上限仍为 64 个绘图节点。

`CONFIG_BSP_LVGL_TASK_STACK_SIZE=12288` 为 LVGL 任务预留 12 KiB 栈。此前的
7 KiB 默认栈在实机 2048 游戏中耗尽，触发栈保护重启并返回开机 ContentView。

两个动画阶段期间，最多八项的 FIFO 按顺序保存 UP/DOWN 移动与 OK 轴切换。
溢出时丢弃最新操作。正常固件关闭逐次移动诊断日志，避免 USB 日志阻塞。长按 OK 仍通过现有页面分发器立即退出，先删除
计时器、丢弃待处理操作，再释放游戏状态。计时器分配失败则退回即时更新。
无效移动不会播放动画或生成方块；重新开始即时执行，分数与页脚反映已提交的
游戏规则状态。合并数字保持可见、背景缩放，新方块数字淡入，文字保持原有 Montserrat 字体大小。


### 可选实机压力诊断

请使用独立构建目录；正常固件默认**关闭**此选项：

```bash
tools/with-env.sh idf.py -B ../work/2048-soak \
  -D SDKCONFIG="$PWD/../work/2048-soak/sdkconfig" \
  -D SDKCONFIG_DEFAULTS="$PWD/sdkconfig.defaults" \
  -D PASSPORT_2048_SOAK=ON build
```

诊断版自动进入 2048，经正常输入队列及页面分发器发送 1,200 次操作，轮换满盘、
密集合并和固定种子棋盘。此构建关闭物理 ADC 输入。串口 `2048-soak` 日志记录
栈余量、空闲堆、LVGL 池完整性和已完成的面板刷新次数；最终
`PASS steps=1200` 表示运行结束。它不测试物理按键触点。
请按适合设备的已验证应用分区流程烧录，测试后刷回关闭此选项的正常构建。
主机 PNG/GIF 的时序不能代表面板实测帧率。

## 预览与测试

```bash
tools/with-env.sh ./tools/test-openswiftui.sh
tools/with-env.sh ./tools/preview-openswiftui.sh ../work/captures/openswiftui-preview.png
```

预览在电脑上编译同一个固定版本的 LVGL，使用真实 C 场景接收器、`ContentView`、
框架模块和图片资源，渲染 240x320 RGB565 像素，再经校验过的截图协议生成 PNG。
其中电量是明确的 **73% 主机测试数据**，不是设备截图。预览还检查按键过滤、配色／图片状态、500 次重绘、未知资源、节点
上限及 20 次页面生命周期与状态重置；需先经 IDF 构建解析出 `managed_components/lvgl__lvgl`。

同一命令还生成采用固定随机种子的新局预览 `-2048.png`、`-2048-horizontal.png`，
以及由明确主机测试棋盘生成的 `-2048-tiles.png`、`-2048-won.png`、`-2048-lost.png`。
另生成明确合并测试棋盘的六帧 `-2048-animation-0.png` 至
`-2048-animation-5.png`，检查中间位置、两个动画阶段、对象复用、FIFO 溢出、
200 次动画移动、动画中途退出、满盘字体边界、500 次重绘、未变帧不触发重绘、
空闲后计时及有界池完整性。同时按主机 16 ms 间隔生成 27 帧
`-2048-motion-00.png` 至 `-2048-motion-26.png`。

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
