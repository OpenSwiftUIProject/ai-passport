简体中文 · [English](README.md)

# AI Passport Embedded Swift 起步工程

基于 [FoloToy/ai-passport](https://github.com/FoloToy/ai-passport) 的 ESP32-C3 固件实验。
现有硬件菜单新增 **Swift** 页面：UP/DOWN 修改 0～999 的计数，OK 在 100% 与 25%
背光间切换，长按 OK 返回菜单并恢复全亮。重新进入页面会重置状态。
电量明确标为 **Boot 启动快照**，不是实时电量。

[验证记录](docs/development/engineering/embedded-swift-validation.zh_CN.md)：
主机测试、ESP32-C3 合并镜像检查、USB 部署和实机启动已通过。
用户已确认可以正常进入 Swift 页面；计数、背光与长按 OK 的人工验收仍待完成。

应用状态和行为使用 Embedded Swift。Swift 直接调用 `bsp_display_backlight()`、
`bsp_battery_soc()`，小型 C 适配层创建 LVGL 页面。现有显示、按键、音频和无线
演示继续保留。永久 Recovery 依赖出厂预装，本镜像不会安装它。
安装此固件会替换 TRAE 出厂应用；本工程未实现出厂的
个人资料和 Token 同步。

## 在此工作区开始

在仓库目录执行：

```bash
swift test                                      # 原生 Swift 逻辑测试
tools/with-env.sh tools/test-swift-interop.sh     # Embedded Swift + C BSP 桩
tools/with-env.sh ./tools/validate.sh             # 完整检查 + 真实 C3 固件
```

增量构建及查看 ELF/map：

```bash
tools/with-env.sh idf.py build
tools/with-env.sh idf.py size
```

完整检查使用隔离配置构建，最终只保存验证过的 `build/FoloToy-AI-Passport-full.bin`。
增量构建还会保留 `build/FoloToy-AI-Passport.elf`、`.map` 和仅含应用的 `.bin`，
便于调试。小程序安装器应使用合并的 `-full.bin`。

### 通过 USB 获取设备截图

运行带截图功能的固件，关闭其他串口监视器后执行：

```bash
tools/with-env.sh python tools/capture_screen.py --output ../work/captures/passport.png
```

工具自动寻找唯一的 Espressif USB 设备；连接多台时用 `--port <实际端口>` 指定。
截图不会重启设备，默认超时 15 秒。只有收到全部像素且每行 CRC32 校验通过后，
才会创建或替换 PNG。PNG 编码仅使用 Python 标准库，`pyserial` 来自 IDF 环境。
截图默认放在 Git 仓库之外。

BSP 在 SPI 字节交换前监听 `LV_EVENT_FLUSH_START`，从现有 20 行绘图缓冲中导出
RGB565 小端像素行。独立 USB 任务持有 LVGL 锁，同步触发一次整屏刷新。
截图时界面可能短暂停顿，按键回调不负责传输数据。MCU 不分配 150 KiB 的整屏
缓存；服务使用 9 KiB 任务栈、2 KiB TX 缓冲、256 字节 RX 缓冲及驱动开销。

`FPS1` 行协议包含独立请求 ID、BEGIN/ROW/END 记录、base64 像素数据和逐行 CRC32。
接收端忽略无关日志与旧请求，拒绝缺失、重叠、越界和设备错误。每个包最多等待
100 ms，传输总预算为 8 秒。超时后会移除临时显示回调并释放 UI 锁。

截图包含 LVGL 渲染的像素及屏幕图层，无法测量物理背光亮度、屏幕坏点，也不是
从 LCD 读回显存。这是按需调试功能，不提供实时视频或远程按键控制。

### 当前设备部署

2026-09-06 对连接的 TRAE 设备做了完整 Flash 备份。它的原始分区表没有 Recovery
条目，`0x700000..0x7fffff` 全部处于擦除状态，因此目前不能使用文档中的永久
BLE Recovery 流程。

本次 USB 测试从已验证的合并镜像中提取应用，**只写入 `0x10000` 的应用**，
继续使用原 bootloader 和分区表。刷写后通过设备摘要校验确认 `0x0..0xffff`
和 `0x310000..0x7fffff` 与备份一致，覆盖 NVS、设备身份及全部资源数据。
原 bootloader 已成功启动 Embedded Swift 应用。

私有备份与串口日志位于 Git 仓库外的 `../work/device-backups/`；当前截图固件位于
`../work/deployment/screenshot-fd28679a/`，最初 Swift 镜像保留在
`../work/deployment/swift-aa3fe0d3/`。本次仅应用 USB 测试不能证明这台设备支持
小程序安装或永久 Recovery。

## 目录

| 路径 | 用途 |
| --- | --- |
| `main/swift/PassportCore/PassportState.swift` | 主机与固件共用的纯 Swift 模型 |
| `main/swift/PassportDemo.swift` | C 可调用的 Swift 生命周期及直接 BSP 调用 |
| `main/swift/PassportBridge.h` | 使用定宽类型的 C/Swift 接口契约 |
| `main/demo_swift.c` | LVGL 对象与物理按键转换 |
| `Package.swift` | SwiftPM 库与主机测试，可用 Xcode 打开开发核心逻辑 |
| `tests/swift-interop/` | 用假的 C 端点执行真实 Embedded Swift 适配层 |
| `tools/with-env.sh` | 仅对当前命令激活 IDF/Swift |
| `tools/capture_screen.py` | USB 屏幕采集、CRC/覆盖范围检查及 PNG 输出 |
| [Embedded Swift skill](skills/embedded-swift-passport/SKILL.zh_CN.md) | 项目工作流，通过 `.agents/skills/` 提供发现入口 |

## 工具链

- ESP-IDF **5.5.3**，ESP32-C3，8 MB Flash，无 PSRAM。
- Managed Component `espressif/idf_swift` **1.0.1**，解析版本记录在 `dependencies.lock`。
- 使用本机已安装的 Embedded Swift **6.3.1 RELEASE**。
  Xcode 自带的 Swift 可运行原生测试，但本机该安装不含所需的 RISC-V Embedded 库。
- CMake **3.29+**、Ninja、Python **3.12**。

工具下载放在 Git 仓库之外的工作区中：

```text
FoloToy/
  ai-passport/                 当前 checkout
  toolchains/esp-idf-v5.5.3/    ESP-IDF 源码
  toolchains/espressif/        编译器、Python 环境和工具
  work/logs/                  搭建及验证日志
```

在新机器上，先从 [Swift.org](https://www.swift.org/install/) 安装支持 Embedded 的
工具链，并准备 CMake、Ninja 和 Python，再执行：

```bash
mkdir -p ../toolchains
git clone --branch v5.5.3 --depth 1 --recurse-submodules --shallow-submodules \
  https://github.com/espressif/esp-idf.git ../toolchains/esp-idf-v5.5.3
export IDF_TOOLS_PATH="$(cd .. && pwd)/toolchains/espressif"
../toolchains/esp-idf-v5.5.3/install.sh esp32c3
```

`tools/with-env.sh` 默认使用同级 `toolchains/`，可通过 `IDF_PATH`、`IDF_TOOLS_PATH`
或 `SWIFT_TOOLCHAIN` 覆盖安装位置。`SWIFT_TOOLCHAIN` 为包含 `usr/bin/swiftc` 的
工具链根目录。脚本不会修改 shell 配置、切换全局 Xcode 或安装全局 skill。

## 测试覆盖与实机验收

原生测试覆盖混合输入、背光状态和计数上下界。互调测试在主机上以 Embedded 模式
执行**真实** Swift 适配层：200 轮进入、操作、退出，未知 action 忽略，可选电量
和背光恢复。它的 C 端点是桩。交叉编译和合并镜像验证覆盖真实 ESP-IDF/BSP 的集成，
这些检查都不能证明板卡实际启动或外设正常。

使用支持数据传输的 USB 线连接设备后：

1. 找到实际 `/dev/cu.usbmodem*` 端口，关闭其他占用串口的客户端。
2. 私下保留官方恢复链接，确认要替换固件后，通过官方小程序或
   [网页工具](https://ai-passport.folotoy.cn/tools/web-flasher/) 安装。
3. USB 串口日志中应出现 `Embedded Swift ready on FoloToy ESP32-C3`。
4. 从初始 Display 菜单选中项按一次 UP 选中 Swift，再按 OK。
   确认计数为 0，电量显示合理的启动读数或 `--%`。
5. 按 UP、UP、DOWN，计数应为 1。OK 应在 25%/100% 背光间切换。
   长按 OK 应恢复全亮并返回菜单，重新进入重测 20 次。
6. 确认没有重启循环、断言、看门狗、持续堆内存损失或原有 demo 损坏。
   对已预装永久 Recovery 及其启动 hook 的设备，关机后按住 UP 开机五秒验证
   Recovery 入口。当前旧出厂布局设备不具备这一前提。

保留 `cardid@0x356000`、`Recovery@0x700000` 和 3 MB 应用上限。
不要对已写入身份的礼物执行整片擦除。以
[BLE/Recovery 兼容规范](docs/development/engineering/ble-recovery-compatibility.zh_CN.md)
为准。设备 SN、KEY、个人恢复链接不应进入仓库。

## 范围

这是本地固件起步工程，不是 SwiftUI 应用，也不是出厂云端协议的实现。
Swift 页面没有新增无线或音频行为。继承的 GitHub 工作流需要在构建环境中安装
Embedded Swift 后才能构建此分支；当前支持路径是本地验证，远端 CI 未测试，
实机结果仅限上面明确记录的检查。

已知构建提示：固定版本的 Swift 组件会传递三个未使用的 `BUTTON_VER_*` C 定义，
触发 Swift 警告；Swift 逻辑不读取这些宏。ESP-IDF 也会把应用与较小的 Recovery
保护分区比较并提示容量不足。本应用实际写入 3 MB factory 分区，不写入 Recovery，
由合并镜像检查验证该布局。

参考：[硬件指南](docs/hardware-design/AI_HARDWARE_DEVELOPMENT_GUIDE.zh_CN.md)、
[上游概览](docs/README.zh_CN.md)、
[乐鑫 Swift 组件](https://components.espressif.com/components/espressif/idf_swift/versions/1.0.1/readme?language=en)。
