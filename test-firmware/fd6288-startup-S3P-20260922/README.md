# S3-P：启动出力 +50% 对照（已实测，不推荐继续使用）

2026-09-22：用户要求做更明显的功率对照。保留 S3 换相/过零/限时规则，
仅增加可选启动功率请求曲线。**已实测、表现倒退、不可飞行**。
首轮 13 次仅第 9 次完成 1 秒闭环观察，7 次未接管、5 次接管后丢反馈。
未接管组滤波最大电流 2.94–3.00 A；不建议继续加功率或重复测试本版。
本轮电压比 S3 低，不能把倒退全部归因于功率变化。完整证据见
[首轮记录](../../hardware-backups/startup-S3P-20260922/README.md)。
2026-09-22 16:52（UTC+8）经用户授权，仅刷主区，一次校验通过。
未全读旧固件，未刷新 BL/F800；RAM 设置前后相同，堵转保护仍 Off。
复位后 variant=8、trial=0、armed=1，输入/输出均零，配对记录为空；
ARR=6665、DTG=0xCE、DWT 正常，停止态约 8.01 V、0.04 A。
操作记录：`hardware-backups/startup-S3P-20260922/flash-1790067138639.json`。

## 出力变化

| 项目 | S3 | S3-P |
| --- | ---: | ---: |
| 获取阶段前 30 ms 请求 | 360（18%） | 540（27%） |
| 获取阶段后续请求 | 400（20%） | 600（30%） |
| EEPROM 对应启动上限 | 610 | 610，不变 |
| 启动输出斜坡初值 | 360 | 360，不变 |
| 接管后助力请求退出 | 从最高 400 退出到 360 | 从最高 600 退出到 360 |
| 退出时长 | 自第六边沿起 20 ms | 同左 |

0..2000 是 duty 标度，括号是名义 PWM 占空比，不是实际功率/转矩百分比。
两档获取请求都 +50%；600 接近现有 610 上限，但该上限不是本试验的
额定安全保证。没有把 PWM 设成 80%，没有增加 EEPROM 启动上限。

实际输出仍从 360 经原有斜坡上升，可能低于请求；温度/电流等已启用
保护形成的更低上限优先，C06 无采样余量时仍停止。不会绕过斜坡直跳
600。原生启动下限/上限设置仍为 360/610，地面站仍显示 Startup power
150；本对照通过编译选项生效，不靠改坏设置范围。

接管后的助力仍在同一 20 ms 时间尺度退出，增量由 +40 改为 +240；
若接管发生在最初 30 ms 内，退出请求不高于 540，也不会在下降后被
强行抬回 540。退出到 360 后恢复原有油门/启动下限逻辑，不维持高助力
整整 1 秒。这个退出段同属功率曲线的变化，需观察是否接管后仍丢反馈。

## 不改变的部分

- S3 首次矢量、方向、前五有效边沿即时推进、第六边沿延迟接管、
  22.5 ms 无反馈试探、真实间隔估计和过零资格全部不变。
- 获取最多 300 ms；成功接管后观察最多 1 秒；闭环首个缺失反馈超时
  即停，不在正油门下自动重启，真实回零才能再试。
- 原生堵转开关/阈值/锁存不改，现有设置仍 Off。软件截止不是堵转
  保护或硬件限流验收，也不替代操作者遇异常回零。
- BL/F800、地面站入口、PWM 频率、死区均不改。

功率升高必然使 PWM 脉宽和原有 CCR4 中点采样位置一起变化。因此即使
改善，也只能先说明这个出力曲线更合适，不能独自证明是转矩不足而非
采样裕量变化。记录里保留真实 duty/CCR/ADC，供后续区分。

## 固件与复现

- 主 HEX：`AM32_CUSTOM_FD6288_G431_S3P_POWER540_600_ACQ300_OBS1000.hex`
- HEX SHA-256：`c4e733f7b88612dbec5a78fb35a0155106375764703b34fd64ecf2304c12f0ff`
- 配对 ELF：`obj-startup-S3P-power/AM32_CUSTOM_FD6288_G431_2.20.elf`
- 有效数据 35612 B，仅 `0x08001000..0x0800F7FF`，无 BL/F800 数据记录。
- BIN 59392 B，SHA-256：`5045e7abeff8e0d292ce0a3085a726ffedf06b5c432d4ceb57aac40f02e39ba6`。
- RAM 23424 B，SP `0x20008000`，reset `0x080061C5`。
- 配对 ABI=1、variant=8。旧摘要字段 `extra=40` 是兼容字段，不表示
  本版助力幅度；读取器显式给出 `power_profile`，不要误读成旧 P1。

```sh
make -j1 CUSTOM_FD6288_G431 OBJ=obj-startup-S3P-power ARM_SDK_PREFIX=arm-none-eabi- FD6288_STARTUP_TRACE=1 FD6288_STARTUP_TIMING_FIX=1 FD6288_STARTUP_SEQUENCE=3 FD6288_STARTUP_PAIR_TRACE=1 FD6288_STARTUP_S3_POWER=1
node tests/run_fd6288_host_tests.cjs
node tests/run_fd6288_pair_tests.cjs obj-startup-S3P-power/AM32_CUSTOM_FD6288_G431_2.20.elf
node tests/check_fd6288_pair_codegen.cjs obj-startup-S3P-power/AM32_CUSTOM_FD6288_G431_2.20.elf
node tests/check_fd6288_image.cjs obj-startup-S3P-power/AM32_CUSTOM_FD6288_G431_2.20.bin obj-startup-S3P-power/AM32_CUSTOM_FD6288_G431_2.20.hex
```

只构建本板型。实际 C 的 S3-P 945 项断言与旧变体回归通过，ASan/UBSan
通过；包含 540/600 请求边界、原斜坡、保护裁剪、20 ms 退出、停止/
重启锁存。variant 6/7/8 实际 C→JS 布局和模拟只读采集测试通过，
构建选项限制检查通过。ARM 日志无 bulk-copy helper，BIN/HEX 一致。
现有 newlib/RWX 链接警告不变；软件测试不验证电机电流/温升或实时裕量。
关闭新选项后 S3 重编译 HEX 与已刷 S3 逐字节一致（SHA
`e4553c8e8b8f2b3a3d1d77f97adb1886cdf2f7ccb22d8d3789804e95c1949bdc`）。

## 实测计划

主区已刷入并完成一次校验，未备份全片或改设置。与上一轮使用
同一负载、油门方式和接近的电池状态；堵转仍先 Off，以便分离变量。
先最多 3 次，有异常电流、温升或明显卡顿立即回零，不等待限时器。
每次完全停稳再试，结束后保持供电读取。不要以原地抖动/仅机械转动
代替完整接管；分别统计未获取、接管后丢反馈、完整观察三类。

```sh
node tools/scripts/fd6288_pair_capture.cjs dump obj-startup-S3P-power/AM32_CUSTOM_FD6288_G431_2.20.elf NEW-S3P-PAIR.bin --stopped
```

配对记录仍保留第一失败/第一完整观察成功，最多 72 扇区，溢出显式标记。
同样不能把 1 秒观察完成当成可飞行或原生堵转保护已通过。
