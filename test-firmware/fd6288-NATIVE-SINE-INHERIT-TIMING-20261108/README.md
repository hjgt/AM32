# NATIVE-SINE：保留原进角和配套配置，2026-11-08

**撤回继续测试：本包未初始化 DWT 控制时钟。请勿再试；后续 CLOCKFIX 包已构建但未烧录。**

分支 `codex/fd6288-native-sine-20261108`。本包取代此前强制 7.5° 的 NATIVE-SINE 包。
已编译与离线检查，尚未烧录。

## 本轮配置

- 启动功率 150%，900 KV，14 极，正弦启动开启，range 15、power 6。
- **进角和自动／手动进角选择沿用原设置，不再由试验代码强制覆盖。**
  本次板上实读为进角编码 18、自动进角 0，即固定 7.5°；这是原参数的值。
- 配置源码为 `Inc/fd6288_native_sine_config.h`；本包的 settings-profile.json
  给出备份来源、修改前后参数、实际修改字节和镜像哈希。
- 新设置页仅有三个字节与原页不同：19（正弦开关）0→1；40（range）5→15；
  45（power）5→6。150%、900 KV 等原本已符合要求，保留原值。
  进角字节 23、自动进角字节 47 和其他所有字节保持原样。

不需要在地面站修改参数。用于一次写入主程序和配置的文件为：

**`NATIVE-SINE-COMBINED.hex`**

包含主区 `0x08001000..0x0800f7ff` 的有效记录，以及设置页
`0x0800f800..0x0800ffff`；不包含 Bootloader。单独主固件和单独设置页也保留。
固件在加载后对 RAM 应用本试验的非进角参数，不自动保存设置或版本升级。
本轮仅生成文件，没有执行擦除、烧录或参数写入。

## 原参数备份与恢复

备份目录：`hardware-backups/NATIVE-SINE-parameters-before-20261108/`。
在匹配板上 EARLY-REKICK 代码后，只读取得完整 2 KB 设置页并重复读取确认一致；
同时保存 192 B 运行参数。没有 halt/reset/写入；保存的运行参数与 Flash 前 192 B 一致。
`capture.json` 记录读取时刻、匹配 ELF 和原始值。

`RESTORE-PREVIOUS-SETTINGS.hex` 是这次原始 2 KB 设置页的逐字节恢复文件，
备份目录也保存一份 `restore-settings.hex`。如果以后刷入了新配置再退回旧程序，
应同时恢复原设置；仅回刷主区不会撤销已经写入设置页的正弦开关。

## ADC 兼容性核对

正弦阶段沿用 AM32 表驱动推进，不依赖比较器或 ADC 过零决定下一次角度。
六步阶段保留本板 ADC 路线：PWM 同步触发三相转换，JEOS 完成中断消费整组新样本，
计算虚拟中性点，按正确方向和前侧→后侧资格报告事件，再通过 TIM16 调度换相。
不是退回主循环反复读取同一 ADC 样本，也不尝试切换到未实现的硬件比较器中断。

交接时关桥、恢复统一六步 PWM／CH4 采样触发、复位 ADC 获取状态，再应用原生
交接扇区。原生的 `zero_crosses=20` 种子不直接变成 ADC 已取得 20 次反馈；
独立计数仍从 0 开始，避免错误施加运行期早事件门限。
首个 ADC 事件不被当作真实的相邻过零周期，第二个真实事件后才建立周期；
这沿用现有 ADC 周期修复，也与原生比较器启动初期的滤波行为存在区别。

新增实际 C 链路测试覆盖：

- 正弦阶段 JEOS 不触发过零换相；交接后选择正确的浮空 B 相及下降方向。
- 边界消隐、只有后侧电平时拒绝、实际前侧武装、两个新后侧样本确认。
- 没有新 JEOS 时不重复读取／计票，一个扇区最多报告一次。
- 从 ADC 判决经过真实 getBemfState、过零处理到异步调度接口；保留原进角计算。

测试使用合成 ADC 电压，不模拟机械转子。它证明代码链路能接通；实际交接的
位置、速度和电压波形是否合适，需要这次实机短测。原生比较器电平判定与本板
ADC 事件资格仍有差异，没有将两者宣称为完全等效。

## 短测和构建

沿用飞控从零缓升油门。整次试验最多 3 秒；得到六次真实反馈后最多再观察 1 秒，
先到者关输出。回零立即结束，保持正油门不自动重启。正常起转后自动停属于预期。
记录只有最近一轮 60 B 阶段摘要，没有新增 ADC 转换或波形记录负担。
这份有界台架版本不用作飞行固件。

```sh
make -j1 CUSTOM_FD6288_G431 OBJ=obj-native-sine-inherit-20261108 \
  ARM_SDK_PREFIX=arm-none-eabi- FD6288_NATIVE_SINE_START=1 \
  FD6288_STARTUP_TIMING_FIX=1 FD6288_ASYNC_COMM=1 FD6288_ADC_JEOS=1 \
  FD6288_ZCD_ACTUAL_DEADTIME=1 FD6288_NO_DIAGNOSTICS=1
node tests/run_fd6288_native_sine_tests.cjs
node tests/run_fd6288_native_sine_tests.cjs --adc
```

实际 C 回归及 ADC 链路测试通过 ASan/UBSan。配置／合并／恢复 HEX 校验与往返比对
通过，主 BIN/HEX 一致；关闭新开关仍与 EARLY-REKICK 归档 HEX 一致。
现有 newlib 系统调用桩及 RWX 链接警告未变。配对 ELF 与哈希见 verification.json。

归零停稳后，使用本分支的只读读取器和本包 ELF：

```sh
node tools/scripts/fd6288_native_sine_capture.cjs \
  test-firmware/fd6288-NATIVE-SINE-INHERIT-TIMING-20261108/AM32_CUSTOM_FD6288_G431_2.20.elf \
  NEW-native-sine.json --stopped
```
