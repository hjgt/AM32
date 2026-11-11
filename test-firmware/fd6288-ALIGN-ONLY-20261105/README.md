# ALIGN-ONLY：双矢量定位后关断

2026-11-05已编译并按用户要求直接烧录；写入和reset run均返回成功，未做读回校验。烧录记录见 [flash.json](../../hardware-backups/ALIGN-ONLY-flash-20261105/flash.json)。用途是观察定位动作，不是可连续运行的启动方案。用户以肉眼反馈转动与最终停点即可，具体解释见 [诊断方案](../../docs/FD6288-ALIGN-ONLY-PLAN-20261105.md)。

## 行为

- ALIGN-PAIR + PB7基线上仅开启 `FD6288_ALIGN_ONLY=1`。默认关闭。完整参数见 [build-args.json](build-args.json)。
- 原第一定位30ms、第二定位60ms不变。名义90ms处PB7拉高，桥不切换；第二矢量继续保持30ms，到名义120ms关断并清低PB7。
- 额外保持仍处于ALIGN、同一功率请求360，遵守既有功率上限；不进入过零获取、不执行首发矢量、不自动重定位/重试。第一段原有功率渐升保留。
- 从trial开始另有120ms软件截止，迟到服务不续期；实际关断存在服务延迟，不能把120ms称为独立硬件定时关断。
- 正常完成记录原因12 `alignment_done`，不记作未锁定失败。停止置inhibit、关桥、清PWM并取消异步换相；正油门保持时不能自行再触发，需操作者归零后重新触发。
- 定位bins保留原90ms窗口，标记时冻结；额外30ms没有新的bins。原配对采样窗口仍不扩展，不把缺少记录当作稳定证据。

## 验证与产物

- 仅构建 `CUSTOM_FD6288_G431`，开启/关闭各一份，日志与参数在 [构建记录](../../docs/align-only-20261105/)。
- 关闭新开关的HEX与已有PB7归档逐字节一致，SHA256 `eeba304cbf3f25b49952e2675a592a86240ae6bf305c6dcfee67bdafeb216a1d`。
- [新策略测试](../../tests/fd6288_align_only_tests.c)在ASan/UBSan下通过：原时长/功率、只打标记不发车、保持上限、不接受定位边沿、不重试、迟到服务、DWT回绕和重新开始。
- `node tests/run_fd6288_async_tests.cjs --align --settle --reposition`通过既有实际调度器/JEOS测试；`node tests/run_fd6288_host_tests.cjs`全部15种既有配置回归通过。上述集成回归不包含新标记GPIO的实物时序验证。
- 镜像检查通过：HEX记录仅在 `0x08001000..0x0800F7FF`，不含bootloader或设置页；BIN/HEX一致。开启版RAM20248B、FLASH代码38312B。既有newlib/RWX链接警告仍在。
- [HEX](AM32_CUSTOM_FD6288_G431_2.20.hex)、[ELF](AM32_CUSTOM_FD6288_G431_2.20.elf)、[BIN](AM32_CUSTOM_FD6288_G431_2.20.bin)、[SHA256](SHA256.json)。HEX SHA256：`543e848e9b194c3edbea6ce46dbe6f32d88fe11bb4151a70c06579a2e3c61757`。
- `source-base.txt`记录Git基点；`source-changes.patch`保存本次构建所需的已跟踪源码差异（包括先前PB7修改），新策略测试另存副本。本次未提交Git。

待实机确认：只有两次定位矢量应用、PB7约30ms高段、关断后正油门不自动重触发，以及用户肉眼观察。不得将离线通过写成定位已稳定。

首轮实测反馈已保存：[肉眼观察记录](../../hardware-backups/ALIGN-ONLY-flash-20261105/RESULT-observation.md)。未改动本目录固件。
