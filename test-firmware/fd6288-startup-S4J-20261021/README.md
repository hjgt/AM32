# S4J：非阻塞换向 / JEOS 对照候选

2026-10-21，仅主固件，**已编译、未烧录、未实机验收，不是飞行版本**。

更新：2026-10-21 23:25（北京时间），用户确认停转后已刷入 JEOS 组合版 variant26；一次校验38008字节通过。重启后输入/运行/PWM均0，armed=1，电压7.88V，async ABI1、pending=0。BL/F800未写。仅完成停机检查，启动与负载尚未验收。日志：[flash.json](../../hardware-backups/startup-S4J-flash-BKF5HH/flash.json)。

| 文件 | variant | 用途 | HEX SHA256 |
|---|---|---|---|
| `AM32_CUSTOM_FD6288_G431_S4J_JEOS_ACQ300_MANUAL_STOP.hex` | 26 | 主测试候选：JEOS＋TIM16 非阻塞闭环换向 | `d4fe0bc7c0bd3d9d6b0528341ca732ef128a2aa5d4097355fb684ec2eea70d2c` |
| `AM32_CUSTOM_FD6288_G431_S4J_ASYNC_ONLY_ACQ300_MANUAL_STOP.hex` | 25 | 对照：原 ADC 轮询＋TIM16 非阻塞闭环换向 | `afba693f8d1fc98b474121f0bac26f35fc1b382ff5cc767abff8890d4aad1f9d` |

只写 0x08001000..0x0800F7FF；不含 BL 或 F800 设置。不要为了测试重新覆盖设置。
两版 RAM 26448B；JEOS 版主 FLASH 段 37504B、HEX 数据共 38008B。

## 改动边界

- variant26 由 ADC1 JEOS 中断唯一消费三相数据；停止原 TIM6 采样消费。TIM6 保留控制/获取期限服务。
- 闭环不再 while 忙等；TIM16 处理单次请求，检查 epoch、方向、coast、零油门和故障门控。allOff/检测器复位/换向取消旧请求。
- TIM6/ADC/TIM16 同级，暂不引入获取状态机的中断嵌套；禁用此实验路径的动态 COM 优先级切换。仍需实测完整 ISR 时长。
- 延时基准仍是软件接受边沿时刻，不做硬件采样年龄补偿或插值。IRQ 到达时间不是转换的准确采样时间，不应据此宣称相位精度已解决。
- 新增实验安全截止：换向回调比计划晚超过 50µs 时停机，记 `late_stop`，沿用 bench 的 LOST_FEEDBACK 停止类别；不是原生堵转保护触发。若出现新停机须一起读取诊断，不能当成新的电机失步。
- 保留 S4I 的首相30ms、首次边沿500µs、后续救援/功率包络、300ms获取截止。闭环没有1s自动停止；操作者回零。原生保护配置不改，不能声称堵转已验收。
- 捕获三rank期间若出现新JEOS会丢弃该组；更完整的触发硬件时间戳、跨PWM延迟识别、物理采样窗口审查仍是后续工作。

## 构建与回滚

在仓库根目录执行：

```sh
node tools/scripts/build_fd6288_s4j.cjs jeos
node tools/scripts/build_fd6288_s4j.cjs async
node tools/scripts/build_fd6288_s4j.cjs off
```

每次只构建 `CUSTOM_FD6288_G431`，对象目录分别为 `obj-startup-S4J-{jeos,async,off}`。
off BIN 与既有 `obj-startup-S4I-first30/AM32_CUSTOM_FD6288_G431_2.20.bin` 经 cmp 逐字节一致。
默认两个新开关均关闭；JEOS 单独打开而不启用非阻塞调度会编译报错。

## 已执行验证

- `tests/run_fd6288_async_tests.cjs`：提取实际调度/IRQ函数，ASan/UBSan；期限、提前回调、迟到停止、回绕、取消、重复、零油门、fault、方向/coast/epoch变化、IRQ mask，以及停止态丢弃/每扇区单次边沿。
- `tests/check_fd6288_s4_flags.cjs`：variant25/26与不合法组合。
- `tests/run_fd6288_split_tests.cjs ELF 25/26`：实际C记录器及JS解码、模拟只读抓取门禁；非真实硬件读数。
- `tests/run_fd6288_host_tests.cjs`：既有0..20所覆盖配置回归通过；不能代替新IRQ的硬件时序测试。
- BIN/HEX地址一致性、主区边界、ARM记录器代码生成检查通过；git diff --check通过。
- 链接仍有既有 newlib 未实现系统调用和 RWX 段警告；无新增编译错误。

## 读数

确认回零停稳后，使用与板上固件一致的 ELF：

```sh
node tools/scripts/fd6288_wave_capture.cjs dump obj-startup-S4J-jeos/AM32_CUSTOM_FD6288_G431_2.20.elf NEW_CAPTURE.bin --stopped
```

原记录器 ABI2不变，解码器已支持25/26，并读取76字节 `fd_async_diag` ABI1。
新增诊断为本次上电累计，不逐试验归属；IRQ计数在零油门也会增长。
重点看 `late_stop`、`stale`、`sample_collisions`、`irq_max_us`、`late_max_us`。
`executed_at` 是换向回调调度时刻，非栅极示波器测得时刻。

首次只做操作者控制的短启动对照；记录失败序号、回零后读取。不同时改功率/timing/堵转设置，不在首轮复现 >50% 油门故障。
