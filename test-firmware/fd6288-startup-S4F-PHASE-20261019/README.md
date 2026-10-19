# S4F-PHASE：启动反馈过渡阶段换相延迟对照

2026-10-19，variant19，已烧录并一次校验通过。基线S4E-LOSS（variant18）。
仅台架对照，不是已验证的修复或可飞行版。

## 唯一控制变量

完全没有有效过零时的开环试探不改：22.5ms等待、一次六段8ms额度。
孤立的第一个合格过零仍立即换相；连续第2～5个合格过零，等待
`min(相邻真实过零间隔 / 2 - 间隔 * min(advance64,24) / 64, 3ms)`
再换相。使用原设置派生的advance64，不更改地面站设置。
3ms只是此次实验上限，不是最佳值结论；等待由现有周期服务执行，
实际时刻有服务周期量化。第六次接管仍用原IIR延迟；之后闭环不改。
超时清除连续计数和实测间隔，下一孤立边沿重新立即换相，不沿用旧延迟。

功率360/400、检测/门限、单次试探预算、原生堵转开关/阈值不改。
等待期间回零取消待换相；已有故障停机与300ms截止仍优先。
改变时机可能间接改变超时次数和堵转触发率，并不代表堵转保护已验收。
未接管300ms停机保留；成功接管后没有1s自动停机，须主动回零。
记录器仍为首次反馈后超时冻结模式，失败和成功分别保留；标识改为19。

## 依据及限制

S4E-LOSS #3：第二边沿后step5消隐后的220个保留采样均为负侧，
没有所需过零，300ms未接管截止。已有证据提示审查换相相位，
不能证明立即换相就是根因，也不能证明增加延迟一定改善。
成功样本也可能先有反馈后超时再恢复，因此本版不新增自动重试或故障判定。

## 文件、构建与验证

HEX：`AM32_CUSTOM_FD6288_G431_S4F_PHASE_ACQ300_MANUAL_STOP.hex`。
SHA256：`9e9ed46c33a38f403f0f3def74f19196e8dd1b4731257a6e17ed2ad8b6727cb1`。
ELF：`obj-startup-S4F-phase/AM32_CUSTOM_FD6288_G431_2.20.elf`。
36048 HEX数据字节；RAM26368 B；SP=0x20008000，reset=0x08006379。
只含0x08001000..0x0800F7FF，不含BL或F800设置。

```sh
make -B -j1 CUSTOM_FD6288_G431 OBJ=obj-startup-S4F-phase ARM_SDK_PREFIX=arm-none-eabi- FD6288_STARTUP_TRACE=1 FD6288_STARTUP_TIMING_FIX=1 FD6288_STARTUP_SEQUENCE=3 FD6288_STARTUP_PAIR_TRACE=1 FD6288_STARTUP_WAVE_TRACE=1 FD6288_ZCD_ACTUAL_DEADTIME=1 FD6288_STARTUP_S4=1 FD6288_STARTUP_HANDOFF_REAL=1 FD6288_STARTUP_RESCUE_RETAIN=1 FD6288_STARTUP_RESCUE_BRIDGE=1 FD6288_STARTUP_ACQ_LOG=2 FD6288_STARTUP_S4E=1 FD6288_STARTUP_LOSS_LOG=1 FD6288_STARTUP_PHASE_DELAY=1
```

默认新开关为0；要求S4E-LOSS基线，其他组合拒绝。关闭开关单target重建
HEX与原variant18逐字节相同。原newlib stub/RWX链接警告仍在。
验证包含实际ADC/主循环适配器回归、延迟前后换相、回零取消待执行工作、
故障/300ms优先、全advance输入/计数器回绕/超时清零、ASan/UBSan、
记录器C→JS、只读抓取模拟、镜像边界及ARM日志代码检查。
这些是软件测试，不模拟实际机械转动。

## 短测与回滚

同设置先3～5次，记录失败序号、是否反向抽动、起转延迟及电流；
异常立即回零，不用增加油门硬顶。停稳保持供电读取，成功也主动回零。
若初轮通过，再分组累积测试，少量样本不能证明成功率超过基线。
回滚HEX：`../fd6288-startup-S4E-LOSS-20261018/AM32_CUSTOM_FD6288_G431_S4E_LOSS_ACQ300_MANUAL_STOP.hex`。
主固件回滚即可，BL/F800不动。

## 实际烧录

用户授权且零油门，停止态门禁通过，只写主固件，36048 B一次校验通过。
首次旧调试连接读取失败，未写入；重连OpenOCD后成功，不重复烧录。
重启variant19、trial=0、armed=1、输入/输出CCR零，设置RAM前后相同。
电压7.82 V，软件电流0.16 A；BL/F800不在写入范围。
日志：`../../hardware-backups/startup-S4F-PHASE-20261019/flash-1792422863813.json`。
等待用户短测，无实机性能结论。
