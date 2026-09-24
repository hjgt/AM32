# S4D-LOG：仅更换启动记录方式

2026-09-24，variant 15，已烧录，待起转实测。控制策略仍为 S4D。
300 ms 获取、接管后 1 s 观察，不能飞行。
只写主固件，一次校验 35084 B 通过，BL/F800 未写，设置 RAM 相同。
重启 variant=15、trial=0、采样间隔=192000 ticks，armed=1，输入/CCR 均零。
日志：`hardware-backups/startup-S4D-LOG-20260924/flash-1790210095817.json`。

## 新线索与目的

用户观察：S4D 某些启动会先轻微反转再正常起转，失败的第七次也如此。
这是用户观察，不是软件测出的方向。可能涉及初始转子摆动或换相与
转子不同步，当前不能确定相序错误、检测极性错误或反转的原因。
第七次 110–147 ms / 239–262 ms 的关键 ADC 已被旧尾部记录覆盖。

这版不再同时试改控制，只更换 ADC 保存方式：

- 同样 256 个 ADC 样本槽，间隔从至少 100 us 改为至少 1200 us。
- 从首个样本开始保存，接管或超过 300 ms 后停止保存；不循环覆盖。
  正常持续提供样本时可覆盖整个获取阶段约 250 个点，RAM 不增加。
- 过零接受事件仍独立记录精确的软件消费时刻和三相 ADC，不受
  波形降采样限制。事件还是 96 条尾环，成功后长时间运行可能覆盖
  早期事件；失败事件也必须检查 overwritten，不能保证任意记录全存。
- 全阶段统计计数、失败/成功独立锁定、16 次摘要保持不变。
- 代价：没有闭环阶段 ADC 尾波形；1.2 ms 是粗采样，不能保证捕获
  亚毫秒毛刺或过零全过程。若仍不能辨别，应进一步定点触发或用
  外部仪器，不能把缺样当没有物理过零。方向 flags 仍是命令而非转向。

未修改 fd6288_startup_sequence.h、main.c 或 comparator.c；功率、
换相、门限、保护、BL/F800 均不改。记录器代码时长可能变化，下一轮
仍要检查 max_sample_cycles/max_event_cycles；这不是绝对零扰动观测。
保留原 ABI 布局与 21568 B 缓冲；variant=15 和 header reserved[0]=192000
明确区分新采样模式，读数必须使用此 ELF 和更新后的脚本。

## 构建

```sh
make -B -j1 CUSTOM_FD6288_G431 OBJ=obj-startup-S4D-acqlog ARM_SDK_PREFIX=arm-none-eabi- FD6288_STARTUP_TRACE=1 FD6288_STARTUP_TIMING_FIX=1 FD6288_STARTUP_SEQUENCE=3 FD6288_STARTUP_PAIR_TRACE=1 FD6288_STARTUP_WAVE_TRACE=1 FD6288_ZCD_ACTUAL_DEADTIME=1 FD6288_STARTUP_S4=1 FD6288_STARTUP_HANDOFF_REAL=1 FD6288_STARTUP_RESCUE_RETAIN=1 FD6288_STARTUP_RESCUE_BRIDGE=1 FD6288_STARTUP_ACQ_LOG=1
```

只构建本板 target；开关默认 0，要求 S4D 配套开关，拒绝错误组合。
匹配 ELF：`obj-startup-S4D-acqlog/AM32_CUSTOM_FD6288_G431_2.20.elf`。
HEX：`AM32_CUSTOM_FD6288_G431_S4D_LOG_ACQ300_OBS1000.hex`。
SHA256：`7ea83c18819201b632cc6456d2f08e100b4478517e9c67edcaf1814d6f60553a`。
35084 数据字节，均在 0x08001000..0x0800F7FF。RAM 26368 B，
SP=0x20008000，reset=0x08005FB5。无 BL/EEPROM 数据。

实际 C 记录器的 ASan/UBSan 测试覆盖：300 ms/DWT 回绕、不覆盖首尾、
接管冻结、事件不受降采样丢失、统计继续、抢占终止、槽锁定、复用、
旧尾部模式回归。C→JS 解码、非法模式/样本拒绝、只读 mock、宏约束、
镜像边界、ARM 事件记录代码检查通过。保留原 newlib stub/RWX 警告。
关闭新选项重建的 S4D HEX 与已实测版本逐字节一致。

## 后续

用户授权烧录后，保持同设置先做少量测试。报告失败序号，以及哪些
成功样本也有短暂反转；不需要徒手拨桨。回零停稳后保持供电读取。
优先比较失败样本首次接受前后的预侧/穿越和下一扇区，而不是只看
成功率。此版没有原生堵转保护实机可用或可飞行的结论。
