# S4D-EDGE：首次接受边沿定点记录

2026-09-24，variant 16，已烧录，待起转实测，控制策略仍为 S4D。
只写主固件，一次校验 35264 B 通过，BL/F800 未写，设置 RAM 相同。
重启 variant=16、trial=0、armed=1，输入及 CCR 均零；100 us 间隔与
12 ms 后触发窗口元数据正确。日志：
`hardware-backups/startup-S4D-EDGE-20260924/flash-1790210893781.json`。
仍为 300 ms 获取/接管后 1 s 观察台架版，不能飞行。

## 依据与范围

S4D-LOG 首轮 #9 失败，45.672 ms 的首个接受边沿距换相约 0.61 ms；
1.2 ms 全程粗采样不能解析其前后。它后面确实继续了 8 ms 短扇区，
但没有形成连续反馈。本版先补局部证据，不继续改变控制。

- 每至少 100 us 保存一次 ADC，256 点循环；首次 ADC 接受时锁定触发。
- 继续保存到触发后 12 ms，之后冻结 ADC 环，不被后续运行覆盖。
  持续采样且历史足够时约保留触发前 13.5 ms 和后 12 ms；实际
  覆盖取决于新样本间隔、触发早晚和停机时刻，必须看时间戳。
- 首次接受的时刻、三相 ADC、elapsed、PWM/trigger、step/命令 flags
  单独保存在每槽原保留字段，后续接受不改写，不依赖事件尾环。
- 如果始终无接受边沿，保留普通 ADC 尾环；统计、96 条事件尾环、
  三类独立锁定及最后 16 次摘要不变。事件仍可能覆盖，不能当全史。
- 不覆盖完整 300 ms，也不保证捕获超过 12 ms 才发生的后继超时。
  这是与上一版全程粗采样互补的局部视图，不是示波器或转子编码器。

未修改启动策略、main.c、比较器、功率、保护或设置/BL。日志首次
接受时增加少量字段写入，时序扰动并非零，实测时检查周期最大值。
RAM 仍 26368 B，记录缓冲 21568 B，不增加数组或采样频率到原尾环以上。

## 构建/验证

```sh
make -B -j1 CUSTOM_FD6288_G431 OBJ=obj-startup-S4D-edge ARM_SDK_PREFIX=arm-none-eabi- FD6288_STARTUP_TRACE=1 FD6288_STARTUP_TIMING_FIX=1 FD6288_STARTUP_SEQUENCE=3 FD6288_STARTUP_PAIR_TRACE=1 FD6288_STARTUP_WAVE_TRACE=1 FD6288_ZCD_ACTUAL_DEADTIME=1 FD6288_STARTUP_S4=1 FD6288_STARTUP_HANDOFF_REAL=1 FD6288_STARTUP_RESCUE_RETAIN=1 FD6288_STARTUP_RESCUE_BRIDGE=1 FD6288_STARTUP_ACQ_LOG=2
```

仅构建 CUSTOM_FD6288_G431。ACQ_LOG=0 为旧尾环，1 为全程粗采样，
2 为本定点记录，仍默认 0。新模式必须与 S4D 组合。
HEX：`AM32_CUSTOM_FD6288_G431_S4D_EDGE_ACQ300_OBS1000.hex`。
SHA256：`0e69ac25819666e24b11ad55766bfb55727a68b83b9e55ba00c101bbdacebe17`。
35264 数据字节，全部在 0x08001000..0x0800F7FF，无 BL/F800。
ELF：`obj-startup-S4D-edge/AM32_CUSTOM_FD6288_G431_2.20.elf`。
SP=0x20008000，reset=0x08006069。

实际记录器 C 的 ASan/UBSan 测试、新模式与原 variant14/15 模式、
早/晚触发、DWT 回绕、环覆盖后冻结、后续边沿不重触发、无边沿尾环、
精确首边沿字段、停止/抢占/槽锁定测试通过。C→JS/非法元数据拒绝、
只读采集模拟、宏组合、HEX 边界及事件记录 ARM 代码检查通过。
保留原 newlib stub/RWX 警告；不是物理电机验证。

## 下一轮

等待用户授权烧录。同样设置，先最多 5 次，记录失败和任何反转序号，
回零停稳后保持供电读取。不必手拨桨，不必为采样延长异常通电。
优先对照首次边沿前的预侧/迟滞和换相后信号，不能仅凭软件 flags
推定机械转向，不能放宽门限把未预备的正侧电压当过零。
