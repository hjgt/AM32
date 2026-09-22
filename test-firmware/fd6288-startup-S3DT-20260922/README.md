# S3-D / S3-DT：S3 原功率，分类型尾段诊断与采样窗口修正

2026-09-22：已完成软件检查，**S3-DT 已实测、启动问题未解决、不可飞行**。
首轮 10 次：3 次完整观察、3 次未接管、4 次接管后丢反馈，体感与 S3
相近。失败 #1 捕获到三十边沿下限撤除后 duty 360→277，紧接着
反馈间隔明显拉长。详见 [首轮证据](../../hardware-backups/startup-S3DT-20260922/README.md)。
23:33（UTC+8）经用户授权，仅刷 S3-DT 主 HEX，一次校验 34612 B
通过；未全读旧固件、未刷新 BL/F800。首次旧连接读失败发生在擦写前，
重启主机调试服务后恢复。复位后 variant=10、trial=0、armed=1，
三槽为空，输入/驱动/CCR 均零；RAM 设置前后相同，堵转仍 Off。
ARR=6665、DTG=0xCE、DWT 正常，停止态约 7.78 V、0.05 A。
成功操作日志：`hardware-backups/startup-S3DT-20260922/flash-1790091180455.json`。
纯诊断对照 S3-D 未刷入。

## 明确修正与未确定的根因

代码检查确认：C03 使实际 TIM1 DTG 固定为 `0xCE`，采样窗口却仍用
旧 `DEAD_TIME=70` 计算。DTG 不是线性计数，`0xCE` 应为
`(32+14)*8=368` 个 t_DTS。本板 CKD=DIV1、PSC=0，所以也是 368 个
PWM 计数（160 MHz 下 2.300 µs）。原 ADC floor=162 比实际死区还早。
依据：[ST RM0440](https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
的 DTG 编码；仓库自带 ST `stm32g4xx_ll_tim.h` 的
`__LL_TIM_CALC_DEADTIME` 也按四段编码而非线性字节值计算。

S3-DT 只修正窗口预算，不修改 BDTR、桥驱动死区或原补偿 200：

| 参数 | S3 / S3-D | S3-DT |
| --- | ---: | ---: |
| 最早 ADC trigger（计数） | 70+92=162 | 368+92=460 |
| 完整三相 ADC 序列预算（计数） | 244 | 244 |
| 最小可采样 PWM 脉宽（计数） | 406 | 704 |
| ARR=6665 时 C06 所需 duty 下界 | 122 | 211 |
| 启动 360 时 CCR / trigger | 1200 / 600 | 不变 |
| 启动 400 时 CCR / trigger | 1334 / 667 | 不变 |

例如 CCR=700 时，旧 trigger=350，可能尚在实际死区；新逻辑要求完整
脉宽至少 704，trigger=460。CCR=800 时 trigger 从 400 改为 460；
CCR>=920 时这项修正不改变原中点采样。C06 沿用同一个窗口下界，
保护上限不足时停机，不能为满足采样而越过电流/温度保护上限。
92 计数的驱动/稳定余量和 244 的转换预算本轮未重新实测，修正公式
不代表已经验证所有模拟裕量。

**这不是已确认的全部启动根因。** 360/400 的获取功率下采样点本来就
足够晚，因此不能用这个问题解释之前所有“未接管”失败；对某次
接管后故障是否有效，也必须先看其实际 duty/采样和丢反馈时间关系。
本轮不把“三十边沿附近失败”或加功率失败直接归因于这一处。

## 诊断缺口的修复

旧配对 logger 只保存首失败和首成功，接管后仅有总数；若首失败是
未接管，其余接管后丢反馈没有细节。S3-D/DT 改为三个独立保留槽：
首个未接管、首个接管后丢反馈、首个完整 1 秒观察。重复同类结果不
覆盖已保留样本；回零提前中止、原生保护/其他安全中止不冒充上述三类。

每槽保留：

- 最近 256 组三相 ADC 结果，按时间间隔至少 100 µs 存储；256 组时
  首尾跨度至少 25.5 ms。记录判定原因、step/预期边沿、预侧见过与否、
  确认数、阶段、计时器、PWM 和 trigger。
- 最近 96 个换相/过零/试探/接管事件；每一个被接受的 ADC 样本也写
  事件，包含其三相原始值，避免恰好落在降采样间隙。
- 全程判定计数和记录器内部耗时最大值；另有最近 16 次试验摘要。

这是**尾段记录**，不是完整启动录波，更不是示波器：覆盖数量明确
输出；成功记录主要是稳定运行尾段，不保证包含早期接管。采样是
软件消费的顺序三相数据，不能独立证明转子方向，不能显示被滤掉的
亚 100 µs 短脉冲。PWM/trigger 为消费时读到的预装载寄存器，不是
ADC 触发当时的锁存值。若仍无法区分模拟瞬态与真实反电势，需要
外部波形证据，不能靠放宽过零条件绕过去。

ST 也明确区分换相退磁阶段与可用反电势区间；退磁过长可遮住过零，
但这只是本项目的待验证假设，不是当前结论。
[ST AN4220 Appendix C](https://www.st.com/resource/en/application_note/an4220-sensorless-sixstep-bldc-commutation-stmicroelectronics.pdf)。

## 不变的控制与边界

S3 原功率 360→400、20 ms 助力退出、首次 step、前五边沿立即换相、
六边沿接管、22.5 ms 无反馈试探、300 ms 获取和接管后 1 秒观察不变。
保持原生堵转开关/阈值/锁存、不改 BL/F800/地面站入口；保护仍待实机
验收。采样改变可能影响过零/超时出现的频率，不能说对保护触发行为
完全无影响，但本轮没有更改保护算法或用台架截止替代保护验收。

## 两份主 HEX（只需按选定版本刷一份）

推荐后续验证候选：

- `AM32_CUSTOM_FD6288_G431_S3DT_WINDOW_ACQ300_OBS1000.hex`，variant=10。
- SHA-256：`f52010a91b82aeeee0a1eb2db1c8e6693eb24cef2b76ddeb8822887e87ed273f`。
- ELF：`obj-startup-S3DT-window/AM32_CUSTOM_FD6288_G431_2.20.elf`。
- 有效数据 34612 B，SP=0x20008000，reset=0x08005DDD。

纯诊断对照，保留原 S3 采样窗口：

- `AM32_CUSTOM_FD6288_G431_S3D_TRACE_ONLY_ACQ300_OBS1000.hex`，variant=9。
- SHA-256：`12b789cf4fb7da96c4e4679e79133544a36046a3f84affecf5ee100086016ca2`。
- ELF：`obj-startup-S3D-wave/AM32_CUSTOM_FD6288_G431_2.20.elf`。
- 有效数据 34588 B，SP=0x20008000，reset=0x08005DC5。

两者仅包含 `0x08001000..0x0800F7FF`，不含 BL 或 F800 记录。
RAM 链接占用均 26360 B（含预留 heap/stack），BSS 结束 0x200060F8；
链接通过不等于实机栈高水位/实时裕量验收。

```sh
make -j1 CUSTOM_FD6288_G431 OBJ=obj-startup-S3DT-window ARM_SDK_PREFIX=arm-none-eabi- FD6288_STARTUP_TRACE=1 FD6288_STARTUP_TIMING_FIX=1 FD6288_STARTUP_SEQUENCE=3 FD6288_STARTUP_PAIR_TRACE=1 FD6288_STARTUP_S3_POWER=0 FD6288_STARTUP_WAVE_TRACE=1 FD6288_ZCD_ACTUAL_DEADTIME=1
```

对照构建使用 `OBJ=obj-startup-S3D-wave FD6288_ZCD_ACTUAL_DEADTIME=0`。
两个新开关均默认 0；禁用后 S3 HEX 与既存基线逐字节一致。
禁止与 S3-P 大功率/其他启动序列组合，避免引入额外变量。

## 验证与后续测试

软件验证：旧控制路径 A/B/P0/P1/S1/S2/S3/S3-P 全部通过；实际 C 的
三槽记录器通过 ASan/UBSan、独立分类/环覆盖/计时回绕/中止/抢占与
无驱动写入测试。C→JS 实际布局、双读稳定和停止态只读模拟采集通过。
窗口测试覆盖全部 256 DTG 编码、65536 脉宽以及 9001 ARR 下的 C06
电流/占空比保护边界。ARM 换相事件记录器无 bulk-copy/函数调用/
栈临时数组；记录器总实时影响仍需上板看，未做硬件耗时保证。
现有 newlib/RWX 链接警告未新增。

待用户授权后只刷选定主 HEX、一次校验，不全读旧程序、不改设置。
第一组最多 5 次，保持负载/油门方式一致，异常立即回零；每次停稳
再试。结束保持供电、不重启，由下列只读工具采集，先确认拿到哪些
类别再决定是否还需补试，不再默认要求十几次。正常接管后约 1 秒
自动停是预期台架行为，不能飞行。

```sh
node tools/scripts/fd6288_wave_capture.cjs dump obj-startup-S3DT-window/AM32_CUSTOM_FD6288_G431_2.20.elf NEW-S3DT.bin --stopped
```
