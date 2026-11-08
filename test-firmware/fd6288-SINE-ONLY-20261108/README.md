# 只运行正弦，不交六步：隔离诊断，2026-11-08

分支 `codex/fd6288-native-sine-20261108`。用户在交接版未改善后，明确要求先看
正弦本身能否把转子带起来。本版使用已经修复并实机确认运行的 DWT 控制时钟。

## 唯一控制差别

保留原生正弦波表、power6、range15、14极、方向、PWM和死区；继续按原来的
油门映射推进角度。越过原交接油门以后，仍在正弦模式，绝不进入六步反馈或超时推进。
最高角度命令速度沿用上一版低速正弦映射上限：每电角度最短57us，不随更高油门
继续变快；不是给转子任意高的开环速度，也不是重新提高出力。

整次有效油门试验最多3秒；回零、失去解锁或时钟／保护故障立即关输出。
截止后保持正油门不重启，需真实回零。没有双矢量定位、反馈换相或补激励。
进角和原参数继续保留；进角不决定本次纯正弦角度推进。

## 肉眼观察

沿用飞控从零缓升油门，只需一轮。看三种情况：

1. 一开始就连续、平顺地转。
2. 前面咔顿，随后才逐渐转顺。
3. 始终一格一格抖，跟不上连续转动。

速度较慢本身属于预期，关键是连续性。最后约3秒关输出、自然停下也属于预期。
这里记录的是命令角度步数，不是机械转速；不要用命令步数推算真实转过几圈。
若一直不顺，只说明这组正弦拖动参数／波形尚未带稳，不能单独认定是转矩不足。

## 记录和复现

仍用60B摘要，没有新增ADC转换或波形缓冲。纯正弦时 `sixstep_at` 应保持未发生，
真实反馈 `edges` 应为0；这两个结果属于本版设计，不表示ADC故障。
归零停稳后，用本包配对ELF和独立分支的 `fd6288_native_sine_capture.cjs` 读取。
读取器额外检查 main 代码身份，避免混用交接版ELF。

```sh
make -j1 CUSTOM_FD6288_G431 OBJ=obj-native-sine-only-20261108 \
  ARM_SDK_PREFIX=arm-none-eabi- FD6288_NATIVE_SINE_START=1 FD6288_NATIVE_SINE_ONLY=1 \
  FD6288_STARTUP_TIMING_FIX=1 FD6288_ASYNC_COMM=1 FD6288_ADC_JEOS=1 \
  FD6288_ZCD_ACTUAL_DEADTIME=1 FD6288_NO_DIAGNOSTICS=1
node tests/run_fd6288_native_sine_tests.cjs --sine-only
```

实际C的低到高油门、两个方向、始终正弦、速度上限、归零／失去解锁／时钟故障与
3秒截止检查通过ASan/UBSan；原交接及ADC链路回归通过。关闭SINE_ONLY后构建的
HEX与已刷CLOCKFIX逐字节相同。BIN/HEX主区范围检查通过。

本包仅主程序，不写Bootloader或设置页。延用当前已装参数；原参数完整备份和恢复
文件仍在 `hardware-backups/NATIVE-SINE-parameters-before-20261108/`。
本文件描述离线准备，烧录及实测状态另记 hardware-backups/SINE-ONLY-flash-20261108/。
