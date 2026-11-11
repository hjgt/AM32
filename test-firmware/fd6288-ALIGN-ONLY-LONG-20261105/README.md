# ALIGN-ONLY-LONG：第二矢量保持300ms

2026-11-05已编译；用户恢复连接后重试，写入及reset run均成功，跳过读回校验。见 [成功烧录记录](../../hardware-backups/ALIGN-ONLY-LONG-flash-20261105/flash.json)。此前连接失败历史：首次reset init失败，未进入flash write_image；重启OpenOCD后仍无法连接目标，ST-Link检测目标电压约3.24V。见 [连接日志](../../hardware-backups/ALIGN-ONLY-LONG-flash-20261105/openocd.log) 和 [首次记录](../../hardware-backups/ALIGN-ONLY-LONG-flash-20261105/flash-attempt1.json)。

## 行为与观察

- 基于已烧录的ALIGN-ONLY短版，第一定位30ms不变，第二定位矢量总保持300ms，名义总通电330ms后关断。全程不发车，不自动重试，沿用原占空比请求和既有保护/停止路径。
- PB7仍在原发车边界约90ms拉高；本版高段名义240ms，约330ms关断时拉低。原始定位bins仍只覆盖前90ms，不扩展额外保持的ADC记录。
- 全trial另有330ms软件截止，迟到服务不续期。它不是独立硬件定时关断，实际服务延迟待实测。
- 配置为FD6288_ALIGN_ONLY_SECOND_MS=300。默认90保留短版行为，配置范围90..300ms，防止无界延长诊断。
- 用户以肉眼观察：大摆是否在开始后立即发生，还是明显停住一会、到关断附近才发生。与原120ms短版相比，观察动作是否跟随关断一起后移，不以启动成功率判断。
- 不要求肉眼与PB7精确同步，不把最终停点视为原90ms状态。改变初始位置后取少量典型观察，避免只从前次停点重复。

## 产物与验证

- 只编译CUSTOM_FD6288_G431；[完整参数](build-args.json)、[构建日志](../../docs/align-only-long-20261105/build.log)。
- 修改后的实际策略测试分别以第二矢量90ms和300ms配置在ASan/UBSan下通过：标记、边界前仍保持、到期停止、定位边沿忽略、不重试、迟到服务和DWT回绕。长期保持配置未在旧120ms截止时提前结束。
- 镜像检查通过：HEX仅覆盖0x08001000..0x0800F7FF，不包含bootloader/设置页；BIN/HEX一致。未进行硬件读回校验。
- [HEX](AM32_CUSTOM_FD6288_G431_2.20.hex)、[ELF](AM32_CUSTOM_FD6288_G431_2.20.elf)、[BIN](AM32_CUSTOM_FD6288_G431_2.20.bin)、[SHA256](SHA256.json)。
- HEX SHA256：c01527cce6e164ddf12d0dee41ef5043ed3a4f7830edce6921242dd49190b27e。
- source-base.txt和source-changes.patch保存Git基点及已跟踪源码差异，策略测试另存副本。既有短版固件归档未覆盖。

首轮肉眼反馈：用户报告不再大摆，像磁铁吸住般稳定，仅有微小振荡。见 [观察与后续建议](../../hardware-backups/ALIGN-ONLY-LONG-flash-20261105/RESULT-observation.md)。本版仍未执行发车验证。
