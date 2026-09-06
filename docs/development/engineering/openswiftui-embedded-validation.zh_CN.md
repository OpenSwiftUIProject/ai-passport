简体中文 · [English](openswiftui-embedded-validation.md)

# OpenSwiftUI Embedded 展示验证

记录日期 2026-09-06，分支 `embed/folotoy`。固件基线 `85382aa`，
OpenSwiftUI 基线 `01cb28f6`，均为本地原型修改。

## Profile 3：物理按键与保留的根视图 State（当前）

当前 ContentView 在根视图声明配色和图片显示两个 `@State` 值。
`.onPhyicButton(.up/.down/.ok)` closure 更新状态，保留的 `EmbeddedViewHost`
失效后重新执行 RootGeometry／布局，再替换场景子节点。UP/DOWN 循环配色，
OK 切换图片，长按 OK 保留全局返回菜单行为；重新进入会重置状态。

应用通过有界、非阻塞队列将 BSP 事件交给 16 ms LVGL timer；页面代次丢弃导航后
遗留的队列事件。没有链接正常 OpenSwiftUI State／GraphHost 实现；本配置只支持
在保留的根视图及存储子视图中构造的 State。`body` 中新构造的有状态子视图、
Binding 投影和通用动态视图身份仍未支持。没有改动其他 framework 仓库。

| 检查 | 结果 |
| --- | --- |
| Build | PASS：完整隔离 ESP32-C3 门禁与合并镜像契约 |
| Host tests | PASS：框架输入／布局／渲染、真实 Swift/C ContentView、队列顺序／溢出／启动／页面代次和分配失败 |
| LVGL 预览 | PASS：500 次重绘、单击过滤、三键操作、场景外壳保留、20 次生命周期状态重置；初始／DOWN／隐藏图片三帧各覆盖 76,800 像素 |
| RISC-V 框架 | PASS：35 个选中源文件，73,838 字节中间静态库 |
| 应用 | PASS：1,557,232 字节，比 profile 2 增加 7,184 字节，低于 3 MB 上限 |
| 正常配置 | 语法解析 PASS；完整桌面构建 NOT RUN |
| Device tests | PASS：仅应用部署／摘要、保护区域、profile 3 启动与三次 CRC 校验 USB 截图；物理按键操作未测试 |
| Unverified | ADC 时序／消抖、物理按键响应、长按 OK 导航、交互时峰值堆、LVGL 任务栈余量及实机反复交互 |

C 原子操作使用 GCC/Clang 内建函数，因为 Swift 组件的 C++ include 路径遮住了
C `stdatomic.h`。最终完整门禁已通过，主机和 ESP32-C3 使用同一份实现。

日志：`FoloToy/work/logs/openswiftui-input-full-validation-final.log`、
`openswiftui-input-preview.log` 和 `openswiftui-input-default-parse.log`。
预览电量为 73% 测试值，是主机渲染，不是设备截图。
暂存：`FoloToy/work/deployment/openswiftui-input-088515ef/`，manifest 保存未提交源码摘要。
应用 SHA-256：`5dc10e17859c2383e347ad1f21a30afa10a71bac78b25fbf011e54c65f739410`。
合并 SHA-256：`088515eff9d83d21a9e6e57f483339d1d6ae869b2e99ffc61bb9229516588636`。
旧设备只有暂存的应用可在重新检查保护区域后写到 `0x10000`；合并文件不用于
这台设备的直接部署。

Profile 3 已于 2026-09-06 部署。写入前备份并校验了当前 3 MB 应用分区，
并私下保存前缀区域（包括当前 NVS）。仅在 `0x10000` 写入
1,557,232 字节应用，写入后应用摘要与两个保护区域校验均通过。
启动渲染 8 个节点、三键驱动初始化成功，场景空闲堆为
158,488 字节；35 秒启动观察内未发现 panic 或 watchdog。

三次无复位 USB 截图各校验了完整 76,800 像素；空闲堆为
150,428 字节，截图工作任务栈余量为
4,300 字节（不是 LVGL 任务栈）。
部署／截图私有记录与回滚快照保存在
`FoloToy/work/device-backups/20260906T044501Z/openswiftui-input-088515ef/`。
本配置的物理按键操作尚未测试。

## 配置 2：测量布局（历史版本）

当前 ContentView 使用 VStack/HStack、padding 与 background，没有 ZStack 或
offset。平台先提供 LVGL 实际屏幕尺寸与主题边距；RootGeometry 将 240x320 转成
216x224 的尺寸提议，测量根视图后将其居中。Text 使用与标签绘制相同的
Montserrat 14 字体及换行规则测量；Image 默认采用固有尺寸，可显式 resizable。

Embedded 的 Layout 协议执行带类型缓存的 sizeThatFits 与 placeSubviews 阶段，
也支持客户端自定义布局。堆叠布局先测量最小／最大弹性，优先为弹性较小的子视图
分配空间，再按声明顺序放置。这里采用从上游 RootGeometry/StackLayout 源码确认
的布局模型，没有链接其 AttributeGraph 实现。整数几何、按索引访问的泛型子视图
及固定默认间距属于本配置的约定；未支持语义见 README。

| 当前检查 | 结果 |
| --- | --- |
| 跨模块渲染／布局测试 | PASS：根边距、变化的尺寸提议、堆叠对齐、弹性、像素余数、溢出、缺席子视图、换行、固有／可缩放图片、修饰器、客户端自定义 Layout |
| 真实 ContentView C 边界 | PASS：8 次绘制、全部绘制失败位置及 begin／geometry／measure 拒绝 |
| 真实 LVGL 主机预览 | PASS：76,800 像素、真实字体换行与标签高度一致、改变显示尺寸、资源／节点限制及 20 次生命周期 |
| 完整门禁 | PASS：主机测试、隔离 C3 构建及合并镜像契约 |
| RISC-V 框架 | PASS：32 个选中源码，中间静态库 65,182 字节 |
| 应用 | PASS：1,550,048 字节，比配置 1 增加 15,040 字节，低于 3 MB 上限 |
| 正常源码配置 | 带 availability 宏的语法解析 PASS；完整桌面构建 NOT RUN |
| Device tests | NOT RUN：构建后未检测到 Passport USB 端口；最近已验证的实机部署仍是配置 1 |

已暂存至 `FoloToy/work/deployment/openswiftui-layout-68e2ba78/`。命名更正也通过
`openswiftui-naming-full-validation.log` 的完整验证，框架使用 `e86ebf2d` 的精确临时导出。重新核验保留区域
后，只能向已有备份的旧布局设备写入 `0x10000` 的应用。当前应用 SHA-256：
`19d7c0a3d694d28408b875a9dcb660fac361f4e6466bb91f9596470bf961589c`。
合并镜像 SHA-256：`68e2ba784902a0a7fa7807d385766ebd4a8046206d731821787dc058044df604`。
静态库体积不等于最终 Flash 占用；测量缓存数组临时存在，配置 2 的 MCU 栈余量、
峰值堆内存及反复进入页面尚未实测。

日志为 `FoloToy/work/logs/openswiftui-layout-host.log`、
`openswiftui-layout-preview.log`、`openswiftui-layout-full-validation.log` 和
`openswiftui-layout-default-parse.log`。主机预览：
`FoloToy/work/openswiftui-preview/layout-host.png`，电量 73% 为测试数据。

## 配置 1：最初静态展示（历史记录）

以下结果与哈希描述此前已部署的配置 1。

| 检查 | 结果 |
| --- | --- |
| Workspace | PASS：七个托管框架 worktree，均为 `embed/folotoy` 分支 |
| 原版 Color 编译探针 | 预期失败：`riscv32-none-none-eabi` 不提供 Foundation |
| Embedded 框架 | PASS：选中 19 个文件，独立模块及 RISC-V 静态库，库文件 28,770 字节 |
| 泛型组合主机测试 | PASS：嵌套 ContentView、条件及可选分支、绘制顺序、frame、offset 与颜色边界 |
| 固件 Swift/C 主机测试 | PASS：真实 ContentView 与框架模块，8 次有序绘制、各接收器失败位置及 begin 拒绝 |
| 无窗口 LVGL 渲染 | PASS：真实 C 场景、主题、图片及 LVGL 9.5；校验 76,800 像素、资源及节点上限拒绝、20 次生命周期 |
| ESP32-C3 增量固件 | PASS：应用 1,535,008 字节，比上一版截图应用增加 3,536 字节 |
| 链接 map | PASS：加载 `libOpenSwiftUI.a(OpenSwiftUI.o)` 并提供 `openswiftui_embedded_version`，包含 Swift Color 符号 |
| 完整门禁 | PASS：仓库与工作流检查、全部主机测试、隔离固件构建及合并镜像契约 |
| 实机部署、设备渲染画面与 USB 截图 | PASS：仅应用刷写、启动渲染、保留区域校验及 3 次完整 USB 截图 |
| 远端 CI 与其他 OpenSwiftUI API | NOT RUN |

28,770 字节是中间目标文件静态库大小，不是最终保留的 Flash 用量。
Embedded Swift 会将泛型框架代码特化到客户端，最终链接删除无用段。
应用体积差还包含页面替换与图片资源，不能视为框架的独立基准测试。

电脑预览使用真实固件 C/LVGL 渲染器与原生 Embedded Swift 模块。其中 73% 电量
是固定测试数据，`free_heap=0` 也是明确的主机桩；两者都不是设备测量。
已检查画面方向、RGB 色条、图片缩放与文字显示。

## 已验证产物

隔离门禁生成 **1,600,544 字节**合并镜像及 **1,535,008 字节**应用。
已在这台旧布局设备上完成仅应用刷写，并通过设备端摘要校验。

```text
Merged SHA-256: ab222746b7fffb2d053df0fa17b10b592d90ce14d8af7018950e240d737e33e8
Application SHA-256: b576edbf9b05f5c52a70ea32d5c1b910e405c8dd29c4a667a9d1e81955552783
Staging: FoloToy/work/deployment/openswiftui-ab222746/
```

暂存目录包含两种镜像及记录源码哈希的 manifest。固件与 OpenSwiftUI 修改尚未
提交。正常配置下的源码语法解析也已通过，但没有构建完整桌面框架。

## 2026-09-06 实机部署

仅向 `0x10000` 写入 1,535,008 字节应用。设备确认为 ESP32-C3、8 MB Flash，
Secure Boot 和 Flash Encryption 均关闭。保留原分区表、bootloader、设备身份、
资源与空白 Recovery 区域，设备端应用摘要校验通过。

刷写前发现 NVS 的 `0xa000` 与 `0xb000` 扇区相较初始完整备份已有 2,069 字节
变化，首次尝试在写入前停止。已私密保存当前 NVS；随后在应用刷写前后确认
`0x0..0xffff` 和 `0x310000..0x7fffff` 两段均与部署前快照一致。
NVS 之外的全部保留数据仍匹配初始备份，没有用原 NVS 覆盖当前设置。

启动日志包含 `profile=1 render=OK nodes=8 free_heap=159796`，35 秒观察内未出现
panic 或 watchdog 标记。3 次 USB 截图与重连各校验 76,800 像素，分别耗时
0.539、0.542 和 0.539 秒；画面完全一致，可用堆均为 151,736 字节，截图任务
栈余量均为 4,296 字节，重连未出现重启标记。已检查实机渲染 PNG 的图片、文字、
RGB 色条、方向及布局。100% 启动电量标签来自设备快照，不代表电池校准测试。

私密证据位于 Git 之外的
`FoloToy/work/device-backups/20260906T044501Z/openswiftui-ab222746/`，包含部署前
保留区域快照、刷写／启动／截图日志、脱敏 JSON 摘要与 `device-screen.png`。
本次验证设备渲染与 USB 截图；未测试物理面板亮度、长时间运行、输入行为或完整
桌面框架构建。

## 裁剪边界

框架选源清单在 `OPENSWIFTUI_EMBEDDED` 配置下编译现有 View/PrimitiveView、
ViewBuilder、EmptyView、Never 与条件内容声明；排除图依赖、MainActor 和运行时
元组元数据，用有序泛型 pair 表示 builder 子节点。Color、Image、Text、ZStack、
固定像素 frame 与 offset 使用小型 Embedded 实现。通过泛型平台接收器同步渲染，
不创建 existential View 树或动态图。

这是 OpenSwiftUI worktree 中新增的实验性 Embedded 配置，没有把正常框架依赖图
移植到 MCU。当前不包含 Foundation、OpenAttributeGraph、OpenRenderBox、
Observation、动态状态、差分更新、动画、SF Symbols、辅助功能或完整 SwiftUI 布局
语义。八个修改过的上游文件在条件编译后保留正常实现，其他同级 worktree 未修改。

## 复现

从固件仓库运行：

```bash
tools/with-env.sh ./tools/validate.sh
tools/with-env.sh ./tools/preview-openswiftui.sh ../work/captures/openswiftui-preview.png
```

框架 `Scripts/test_embedded.sh` 跨真实模块边界测试泛型渲染；
`tools/test-openswiftui.sh` 还编译固件实际 ContentView 和 C 边界测试代码。
无窗口预览编译真实 LVGL 场景，不将绘图方法替换为桩。

构建日志保存在 Git 之外的 `FoloToy/work/logs/`：
`openswiftui-embedded-host.log`、`openswiftui-embedded-riscv.log`、
`openswiftui-firmware-build.log`、`passport-contentview-interop.log` 和
`openswiftui-lvgl-preview.log`、`openswiftui-full-validation.log` 和
`openswiftui-default-parse.log`。

设备部署约束与准确 API 范围见[当前 README](../../../README.zh_CN.md)。
此前设备备份及保留分区检查见[最初验证记录](embedded-swift-validation.zh_CN.md)。

参考：[Embedded Swift 链接模型](https://forums.swift.org/t/embedded-swift-linkage-model/81441)、
[Embedded Swift ABI](https://docs.swift.org/latest/documentation/embeddedswift/abi/)。
