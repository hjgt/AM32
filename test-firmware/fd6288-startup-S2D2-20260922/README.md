# S2D2：只优化换相记录初始化，不改启动控制

状态：已构建、离线测试通过，**尚未刷入，实际记录耗时待复测，不可飞行**。
板上仍为 S2D。新的详细数据见
[S2D 六次测试](../../hardware-backups/startup-S2D-20260922/README.md)。

## 改动与未改动

S2D 实测换相记录边界最大 1983 cycles（12.394 µs），超过预设检查线。
反汇编看到该路径栈上构造结构，再经 memset/memcpy 复制到记录区。
现在改为直接初始化每个 volatile 字段，不构造临时整块记录。
复用记录区也初始化所有 128 B 字段；没有偷省计数清零或牺牲信息完整性。

没有改动启动功率、30 ms 定位、加速/保持规则、接管条件、300 ms 截止、
接管后 1 秒观察、过零检测、保护阈值、地面站入口、BL 或 F800 设置。
pair ABI=1、variant=6 不变，旧读取工具可用，但**必须换配对 ELF**。
不能据本次优化宣称电机起转问题已经修复，也未证明诊断开销是失败根因。

## 主文件

- `AM32_CUSTOM_FD6288_G431_S2D2_PAIR_ACQ300_OBS1000.hex`
- SHA-256：`dd435fd0d3096893d26e9f6943700dfc182ded9d6d8683dbf4fe4953e93c779e`
- 配对 ELF：`obj-startup-S2D2-direct/AM32_CUSTOM_FD6288_G431_2.20.elf`
- HEX 35096 B，仅主区 `0x08001000..0x0800F7FF`；RAM 23424 B。
- BIN/HEX 一致，BL/F800 无数据记录，链接器现有 newlib/RWX 警告不变。

## 验证

```sh
make -j1 CUSTOM_FD6288_G431 OBJ=obj-startup-S2D2-direct ARM_SDK_PREFIX=arm-none-eabi- FD6288_STARTUP_TRACE=1 FD6288_STARTUP_TIMING_FIX=1 FD6288_STARTUP_SEQUENCE=2 FD6288_STARTUP_PAIR_TRACE=1
node tests/check_fd6288_image.cjs obj-startup-S2D2-direct/AM32_CUSTOM_FD6288_G431_2.20.bin obj-startup-S2D2-direct/AM32_CUSTOM_FD6288_G431_2.20.hex
node tests/check_fd6288_pair_codegen.cjs obj-startup-S2D2-direct/AM32_CUSTOM_FD6288_G431_2.20.elf
node tests/run_fd6288_pair_tests.cjs obj-startup-S2D2-direct/AM32_CUSTOM_FD6288_G431_2.20.elf
node tests/run_fd6288_host_tests.cjs
```

只编译本板型。实际 C ASan/UBSan 通过；新增“用 0xA5 污染复用记录，再与
旧初始化约定逐字节比较全部 128 B”的回归通过；配对保留/中断入口抢占/
溢出/解码布局/只读模拟 RPC 测试及原 A/B/P0/P1/S1/S2 控制回归通过。
ARM 反汇编检查确认 `fdTraceEvent` 无函数调用、无 bulk-memory helper、
无额外栈上临时结构分配（仍有正常寄存器保存）。**这不是实测耗时证明。**

## 下一步（未执行）

按授权主区刷写、单次校验和停止态检查之后，先一次短试，回零停稳、
保持供电，读最大采样/边界耗时与完整性标志。不要求再先跑十次。
若边界仍接近/超过 5 µs，继续评估记录开销；低于检查线也不等于整个
ISR/ADC 时序已经验证。若记录质量合格，再收集少量成功/失败对照。
卡顿或异常电流立即回零，不为日志强行延长通电。

```sh
node tools/scripts/fd6288_pair_capture.cjs dump obj-startup-S2D2-direct/AM32_CUSTOM_FD6288_G431_2.20.elf NEW-S2D2-PAIR.bin --stopped
```

S2D 证据提示至少两类情况：第 1 次末段资格达到 5，下一边沿在扇区
92% 处被拒绝；第 6 次全程仅两个接受边沿。后续分别核查相位跟随和
初始定位/带转/检测，不能仅放宽接管或增大功率同时处理所有失败。
