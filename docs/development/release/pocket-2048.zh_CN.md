<p align="right">
  <strong>简体中文</strong> · <a href="pocket-2048.md">English</a>
</p>

# 口袋 2048 发行版

一款带动画的数字合并小游戏：滑动相同数字，累积分数，挑战 2048。
发行版开机直接进入游戏。

## 玩法

| 按键 | 纵轴模式（默认） | 横轴模式 |
| --- | --- | --- |
| UP | 向上移动 | 向左移动 |
| DOWN | 向下移动 | 向右移动 |
| 短按 OK | 切换到横轴 | 切换到纵轴 |
| 长按 OK | 离开游戏，返回 Demo 菜单 | 离开游戏，返回 Demo 菜单 |

底部显示当前按键方向。切轴不会移动或生成方块。有效移动后会出现新方块，
相同数字在一次移动中最多合并一次。滑动和合并动画让每一步更容易看清。
达到 2048 或无法继续移动后，按 OK 重新开始。退出或关机会丢失当前这一局。
没有存档、撤销或排行榜，不需要联网。

## 选择正确文件

- `*-full.bin`：完整合并固件，用于官方 AI Passport 社区上传。支持的官方
  安装流程解析它，并保留设备身份与出厂 Recovery；固件不包含设备专属身份
  数据，也不替换 Recovery。
- `*-app.bin`：仅应用固件，供已核实分区兼容的开发者使用；应用位于
  `0x10000`，大小上限为 `0x300000`。不能把这个文件上传到社区。
- `manifest.json`：准确的源码 revision、构建配置和固件哈希。
- `SHA256SUMS`：解压后各文件的校验和。macOS 使用
  `shasum -a 256 -c SHA256SUMS`，Linux 使用 `sha256sum -c SHA256SUMS`。

具有出厂 Recovery 的设备请按官方社区／小程序安装指引操作。
此候选版本尚未测试社区安装。早期 TRAE 礼品设备可能采用不同分区且没有
Recovery：不要全片擦除或从 `0x0` 写入完整固件。对这类设备，应先私下备份
整片闪存并核实分区，再将 `*-app.bin` 仅写入 `0x10000`，并检查应用范围之外
的所有字节保持不变。发行包不包含任何设备备份。

## 复现候选版本

克隆 `https://github.com/OpenSwiftUIProject/ai-passport`，检出
`manifest.json` 中的 `source.revision`。按根 README 安装 ESP-IDF、Embedded
Swift 工具链并 setup OpenSwiftUI，再在 OpenSwiftUI 仓库检出 manifest 中的
`openswiftui.revision`，运行：

```bash
tools/with-env.sh tools/validate.sh --release-2048
```

它先运行全部主机测试，再从隔离的默认配置构建，明确启用
`PASSPORT_BOOT_2048=ON`、关闭 `PASSPORT_2048_SOAK=OFF`。验证完整固件、
保护区域、应用大小和 Recovery hook 后，在
`../work/releases/pocket-2048-<revision>/` 生成 ZIP 和校验和。
源码必须已提交且干净，目标目录必须不存在。构建时间和本地工具链路径可能影响
二进制字节；manifest 用于识别交付的准确固件。普通 `tools/validate.sh`
仍然构建开机进入静态 Demo 的开发版。

## 发布截图

固件同时支持官方 `FAP_SCREENSHOT_V1` USB 截图请求和现有 FPS1 工具。
持有 LVGL 与控制台锁时，复用分块绘图缓冲传输一帧 RGB565 数据，不分配完整
帧缓冲。传输限时八秒，主机断开不能无限阻塞 UI。截图工作任务的栈为配置的
LVGL 栈大小加 2 KiB。截图不会修改游戏状态、重启设备、更改设置或读取设备凭据。

从 `https://ai-passport.folotoy.cn/skills/folotoy-ai-passport-publisher.zip`
安装官方发布 skill，依次进行 `capture-screen`、`validate`、授权及预览。
封面必须有近期真实设备截图及其回执。主机预览属于测试素材，不能充当串口连接
证据。完整预览经过审核确认后才能上传。
