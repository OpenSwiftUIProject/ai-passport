简体中文 · [English](embedded-swift-validation.md)

# Embedded Swift 验证记录

记录日期：**2026-09-06**。本地分支 `feature/embedded-swift-starter`，基于上游
`c73254e2f6a142bcafca2683056845c6337cafb2`。记录的是本地起步工程实现，
不是已发布版本。

| 检查 | 结果 |
| --- | --- |
| 环境 | PASS：macOS 26.6.2 / Apple Silicon、Swift 6.3.1 RELEASE、ESP-IDF 5.5.3、GCC 14.2.0、Python 3.12.13、CMake 4.4.0 |
| Swift 模型主机测试 | PASS：3 个 XCTest 用例 |
| Embedded Swift C 互调 | PASS：真实 Swift 适配层、假的 C 端点，正常与不可用电量场景共 200 轮页面生命周期 |
| 现有 C/Python 测试 | PASS：界面计算及 2 个固件验证器测试 |
| 截图主机测试 | PASS：Python 接收真实固件 C 编码结果；5 个测试覆盖像素/PNG 颜色、CRC、边界、旧记录、缺失与重叠 |
| USB 截图 | PASS：20 次重连并截图，每帧 76,800 像素；含完成诊断耗时 0.557～0.567 秒 |
| 截图内存 | PASS：20 次抓取的空闲堆始终为 151,736 字节；任务栈最小余量 4,520 字节 |
| 接收端中断与异常输入 | PASS：中途关闭、重连，忽略非法及过长命令后成功获取完整帧，无重启 |
| 仓库、工作流和静态检查 | PASS |
| Skill 格式检查 | PASS |
| ESP32-C3 固件 | PASS：RISC-V 32 位 ELF，链接 Swift 生命周期函数及 C BSP |
| 完整验证 | PASS：`tools/with-env.sh ./tools/validate.sh` |
| USB 部署与启动 | PASS：ESP32-C3 revision v1.1、8 MB Flash；仅写应用并校验摘要，观察到 Embedded Swift 启动标记 |
| BSP 初始化 | PASS：实机报告 Display=1、Button=1、Audio=1、Battery=1；这不是画面或音频验收 |
| 最初固件空闲运行观察 | PASS：串口采集 240.2 秒，一次启动，没有 panic 或看门狗标记；未观察到 Swift 页面输入事件 |
| Swift 页面进入 | PASS：用户确认正常进入测试页；反馈发生在串口采集结束之后 |
| 计数、背光与长按 OK 验收 | PENDING：这些具体行为尚未得到确认 |
| 永久 Recovery | 刷写前已不可用：无分区条目，整个预留区域处于擦除状态 |
| 远端 GitHub CI | NOT RUN：继承的环境仍需安装 Swift |

## 截图固件

已部署的截图应用为 **1,531,472 字节**，小于 factory 分区的 **3,145,728 字节**；
合并镜像为 **1,597,008 字节**。

```text
build/FoloToy-AI-Passport-full.bin
Merged SHA-256: fd28679a152c003a41acb03d94b82c8b84d6a5855b33b87ad413aeb2f77bf368
Application SHA-256: 0e23cc1939f7daa88b97393249512b52d05dc9e7931f34091e83a40f606d04b4
```

部署前完整门禁通过。仅更新 `0x10000` 应用，刷写前后的设备摘要校验确认原
bootloader、分区表、NVS 和 `0x310000..0x7fffff` 数据保持一致。最终电脑端接收工具
也已通过完整静态门禁。

为避免连接时在 macOS 上触发 `USB_UART_CHIP_RESET`，接收工具采用
ESP-IDF Monitor 的无复位顺序：打开前同时置位，打开后先释放 RTS，
再释放 DTR。三次仅连接测试及 20 次重连截图均未出现启动或 panic 标记。

中途关闭接收端后观察到有时限的 `ESP_ERR_TIMEOUT`；新请求在 0.684 秒内恢复
全部 76,800 像素，空闲堆未变。这验证了停止接收后的恢复，没有测试物理拔线。
MCU 不分配 150 KiB 截图缓存。已抓取真实 LVGL 主菜单，并检查 PNG 的颜色、方向
和布局；物理亮度及屏幕坏点不在截图验证范围内。

私有证据位于 `FoloToy/work/device-backups/20260906T044501Z/screenshot-fd28679a/`，
包括 `boot.log`、`stress-results.json`、`serial-open-check.json` 和
`interruption-results.json`。构建及主机日志为
`FoloToy/work/logs/usb-screenshot-full-validation.log` 和
`FoloToy/work/logs/usb-screenshot-static.log`。

## 最初 Swift 固件

最初应用为 **1,519,392 字节**，合并镜像为 **1,584,928 字节**，保留用于回退：

```text
../work/deployment/swift-aa3fe0d3/FoloToy-AI-Passport-full.bin
SHA-256: aa3fe0d3ace2697fd15f0dedae8d15c39b6b2a75b748032ab4d8b59825a1fc2f
```

完整验证已检查分区 MD5 和布局、应用偏移 `0x10000`、3 MB 上限、设备身份与
Recovery 保护范围及其数据排除，并确认社区镜像中五秒 Recovery 启动 hook 已链接。

## 2026-09-06 USB 部署

写入前已读取完整的 8,388,608 字节 Flash 备份并计算摘要。设备未启用 Secure Boot
或 Flash Encryption。原出厂布局包含 `imgstore`、`imgframe`、`cardid`、`audio`
和 `imgava`，没有 Recovery 条目；部署前 `0x700000..0x7fffff` 全部为 `0xff`。

本次只把经过验证的应用写到 `0x10000`，保留原 bootloader 与分区表。
应用 SHA-256 为
`d07b6538cb67e86fc086954a1b02ee87b2712ed2ccb80ad8f1e0e5a6cc9a3bfe`。
设备摘要校验确认 `0x0..0xffff` 和 `0x310000..0x7fffff` 与备份一致，
覆盖原 bootloader、分区表、NVS、全部资源、设备身份和空白 Recovery 区域。

原 bootloader 已从 `0x10000` 加载新应用。日志确认 ESP-IDF 5.5.3，出现
`Embedded Swift ready on FoloToy ESP32-C3`，随后报告 Display、Button、Audio
和 Battery 初始化成功。社区 bootloader 的 Recovery hook 没有安装到这台设备，
也没有进行实机验证。

私有证据位于 Git 仓库外的 `FoloToy/work/device-backups/20260906T044501Z/`：
完整备份、manifest、`flash.log`、`preservation.log` 和 `swift-runtime.log`。
待刷分段从记录的合并镜像提取，放在 `FoloToy/work/deployment/swift-aa3fe0d3/`，
避免与之后增量构建的文件混用。

最初增量 `idf.py size` 报告静态 DRAM 使用 179,112 字节。显示的 142,184 字节余量
是链接阶段预算，**不是实测运行时空闲堆**。长时间碎片情况与多外设组合负载仍未验证；
截图测试的实测空闲堆记录在上文。

map 包含 `passport_swift_prepare`、`passport_swift_enter`、`passport_swift_action`、
`passport_swift_exit`、`bsp_display_backlight` 和 `bsp_battery_soc`。
Swift 已进入固件镜像，而非仅存在于主机测试包。

本地证据放在 Git 仓库之外的 `FoloToy/work/logs/`：`idf-install.log`、
`swift-firmware-build.log`、`swift-full-validation.log`。
生成的固件、ELF 和 map 放在已忽略的 `build/` 中。

版本宏与 Recovery 大小提示见[起步工程 README](../../../README.zh_CN.md)。
确认屏幕、输入、背光、电量或 Recovery 实际可用前，应执行其中的实机检查。
