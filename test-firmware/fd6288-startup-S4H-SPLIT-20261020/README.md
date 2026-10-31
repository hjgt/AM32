# S4H-SPLIT：双窗口启动记录器（variant23/ABI2，已烧录）

2026-10-20。用户授权仅改记录器，控制基线仍是板上的S4H-RAMP。
不改初始施力、功率、六段补救、过零资格、换相延迟、接管或保护。
300ms未锁定截止保留；正常接管后没有1s自动停机，需主动回零。
不是启动修复版、可飞版本或堵转保护验收版。

## 为什么改

S4H第4次失败：六段补救没有合格边沿，170ms才出现孤立边沿。
原混合速率环被之后高频采样覆盖，只剩133ms之后数据，无法检查
最初施力。现在把原256采样槽拆成两个独立128槽区域，不增加RAM：

1. **起步前缀**：<=50ms、相邻点至少400us、最多128点，永不覆盖。
   即使过零或接管很早发生也继续至50ms（若trial先结束则提前停止）。
2. **首边沿窗口**：从起步按至少100us滚动采样，首次接受后再记录
   最多6ms；原反馈超时/接管或trial结束若更早则提前冻结。
   128点通常涵盖首边沿前约6.7ms及之后6ms，实际跨度取决于采样间隔。
   首边沿原始ADC和时间仍独立精确保留；后续边沿不会再次触发窗口。
   没有边沿时仅保留<=300ms的末尾环，不再保留300ms完整粗采样。
3. **事件**：保留最初96条，不再循环覆盖，超过容量以`omitted_suffix`
   明示丢失后续事件。摘要计数/最终停止原因仍独立保留，不保证所有
   trial的完整事件序列都能容纳。正常闭环大量事件不会覆盖起步序列。

解码分别输出`startup_prefix`和`first_edge_window`。兼容汇总`samples`
可能包含两个窗口的重叠时间点，分析时优先看独立窗口。
三类首样本锁定和最后16次摘要规则不变。波形不是连续示波器测量；
400us/100us间隔可能漏掉短暂变化，不能单独证明转子方向。

ABI2仍为21568字节：bank的`samples_stored`低16位为前缀数、
高16位为细节环累计写入数；JS转换后保留原始`packed_sample_counts`。
旧ABI1/variant9..22独立解析，旧工具不能解码本版。

## 产物

- 主固件：`AM32_CUSTOM_FD6288_G431_S4H_SPLIT_ACQ300_MANUAL_STOP.hex`
- SHA256：`89d63ebc1054d91e452962d00147b34935294f6b2c53b45940752f4adefde566`
- ELF：`../../obj-startup-S4H-split/AM32_CUSTOM_FD6288_G431_2.20.elf`
- HEX36504数据字节，范围0x08001000..0x0800F7FF，不含BL/F800设置。
- RAM26368字节不增；初始SP0x20008000，reset0x08006531。

单目标复现：

```sh
make -B -j1 CUSTOM_FD6288_G431 OBJ=obj-startup-S4H-split ARM_SDK_PREFIX=arm-none-eabi- FD6288_STARTUP_TRACE=1 FD6288_STARTUP_TIMING_FIX=1 FD6288_STARTUP_SEQUENCE=3 FD6288_STARTUP_PAIR_TRACE=1 FD6288_STARTUP_WAVE_TRACE=1 FD6288_ZCD_ACTUAL_DEADTIME=1 FD6288_STARTUP_S4=1 FD6288_STARTUP_HANDOFF_REAL=1 FD6288_STARTUP_RESCUE_RETAIN=1 FD6288_STARTUP_RESCUE_BRIDGE=1 FD6288_STARTUP_ACQ_LOG=2 FD6288_STARTUP_S4E=1 FD6288_STARTUP_LOSS_LOG=1 FD6288_STARTUP_PHASE_DELAY=1 FD6288_STARTUP_SEED_DELAY=1 FD6288_STARTUP_NOEDGE_LOG=1 FD6288_STARTUP_RESCUE_RAMP=1 FD6288_STARTUP_SPLIT_LOG=1
```

## 验证与注意

实际C记录器ASan/UBSan测试、晚/早/无边沿、两种事件冻结、时间截止、
DWT回绕、锁定/复用/抢占、事件容量溢出、C→JS布局与损坏输入拒绝通过。
新ELF只读mock抓取通过；原variant22记录器/解码/mock回归通过。
目标宏、镜像边界、ARM事件日志无函数调用/额外栈临时检查通过。
采样函数无函数调用，有44字节局部栈；尚未测实机完整ISR耗时。
关闭新开关重建与S4H-RAMP HEX逐字节一致。原newlib stub/RWX警告保留。

这是日志开销变化，不保证绝对零时序扰动。下次抓取检查
`max_sample_cycles`/`max_event_cycles`并与旧版对照。

## 测试与回滚

保持设置不变，先取得正常样本，再遇到一次失败即回零停稳、保持供电
读取。若始终正常不必长时间反复带载。正常样本若未达到完整1s观察，
未必会固定到success槽；不要为了凑记录延长异常运行。
读数命令（仅确认停稳后）：

```sh
node tools/scripts/fd6288_wave_capture.cjs dump obj-startup-S4H-split/AM32_CUSTOM_FD6288_G431_2.20.elf hardware-backups/startup-S4H-SPLIT-20261020/first-trials-read.bin --stopped
```

回滚使用S4H-RAMP测试包HEX。此次未访问硬件、未烧录；板上仍variant22。

## 后续烧录：2026-10-20 23:29（北京时间）

前两轮连接失败均在任何擦写前退出；用户再次要求重试后通信恢复。
停止态检查通过，只写主固件，一次校验36504字节通过。
重启variant23/ABI2、trial0、armed1，油门/CCR零，8.30V、软件电流0.02A。
记录器时序字段16000/960000/64000符合预期；无旧固件备份或整片读回，
BL/F800不在写入范围。板上现为S4H-SPLIT，等待实机正常/失败配对记录。
日志：`../../hardware-backups/startup-S4H-SPLIT-20261020/flash.json`。
