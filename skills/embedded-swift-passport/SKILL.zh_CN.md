---
name: embedded-swift-passport
description: 使用 C BSP 和 LVGL 桥接，在本 FoloToy AI Passport ESP32-C3 工程中开发和测试 Embedded Swift 固件。
---
简体中文 · [English](SKILL.md)

# AI Passport Embedded Swift 开发

本 skill 用于此工程的固件修改。先阅读根目录 `AGENTS.md` 和
[工程 README](../../README.zh_CN.md)。官方内容推送 skill 操作出厂应用的
内容队列，属于另一条集成路径。

- 使用 ESP-IDF **5.5.3**、`espressif/idf_swift` **1.0.1**，以及包含
  `riscv32-none-none-eabi` 库的 Embedded Swift 工具链。本机选择和搭建命令见 README。
- 纯逻辑放在 `main/swift/PassportCore`，固件入口放在 `main/swift/PassportDemo.swift`，
  C 界面适配放在 `main/demo_swift.c`，板级驱动放在 `components/bsp`。
  SwiftPM 与 ESP-IDF 编译同一份核心源码，不要为主机另写一份模型。
- `PassportBridge.h` 定义 C/Swift 边界，保持定宽参数和 C 链接。
  可变参数宏或无法直接导入的接口用小型 C 包装，普通 C BSP 函数可以直接调用。
- `prepare` 在启动期间、LVGL 上下文之外执行。页面进入、按键和退出由菜单持有
  LVGL 锁后调用。不要向按键回调加入阻塞的 I2C、音频、存储或网络操作。
  当前电量是启动快照，有意在界面启动前采样。
- 除非用户要求不同导航，保留官方菜单及长按 OK 返回语义。退出页面时恢复默认背光。
- 保留固件兼容规范定义的设备身份、Recovery 保护区、恢复启动 hook 和 3 MB 应用上限。

## 按修改边界验证

1. `swift test` 在主机验证不依赖硬件的逻辑。
2. `tools/with-env.sh tools/test-swift-interop.sh` 在 Embedded 模式编译真实适配层，
   使用假的 C BSP/UI 端点在主机执行。
3. `tools/with-env.sh ./tools/validate.sh` 运行仓库检查、主机测试、ESP32-C3 构建
   和合并镜像兼容验证。
4. 有设备后执行 README 的实机检查，分别报告主机执行、交叉编译和板卡观察结果。

不要把主机桩或 RISC-V 编译描述成对实体屏幕、ADC、电量计的验证。
运行日志、屏幕行为、Recovery 入口需要实机检查。设备凭证不得进入源码或文档。
环境搭建命令不代表授权推送、发布或破坏性刷机。
