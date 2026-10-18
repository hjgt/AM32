# S4E-LOSS：获取阶段首次反馈中断窗口

2026-10-18，variant18，已烧录并一次校验通过。只改记录器，控制策略仍S4E。
**未接管300ms停机；接管后无1s自动停机，须主动回零；不可飞行。**

## 改动

保留256点、至少100us采样尾环。出现至少一个被接受的ADC边沿后，
获取阶段第一次TIMEOUT冻结ADC；若先成功接管则在HANDOFF冻结。
此前完全无反馈的盲试探超时不冻结；始终无边沿保留普通尾环。
后续边沿、超时、运行均不覆盖冻结波形；每次新启动重置冻结状态。
所有计数/事件继续按原S4E的诊断窗口工作，首边沿精确字段仍保留。

这能覆盖本轮第二边沿至step5等待22.5ms的整段；256点窗口通常
约26ms，具体以时间戳为准，不承诺保存整个300ms、首次边沿之前
全部波形或物理转向。第一次超时后若再次发展出另一种失败，不会
重新触发。首次失败/首次成功分类锁定仍不变。

不改 main.c、启动策略、比较器、功率、门限、原生保护、BL/F800。
原生堵转尚无实机可用结论。记录开销变化需实测，不能声称零扰动。
冻结原因写每槽保留字段高字节，解码显示trial_end/first_post_feedback_timeout/
handoff；其他字段布局不变，RAM总占用26368B。

## 构建

```sh
make -B -j1 CUSTOM_FD6288_G431 OBJ=obj-startup-S4E-loss ARM_SDK_PREFIX=arm-none-eabi- FD6288_STARTUP_TRACE=1 FD6288_STARTUP_TIMING_FIX=1 FD6288_STARTUP_SEQUENCE=3 FD6288_STARTUP_PAIR_TRACE=1 FD6288_STARTUP_WAVE_TRACE=1 FD6288_ZCD_ACTUAL_DEADTIME=1 FD6288_STARTUP_S4=1 FD6288_STARTUP_HANDOFF_REAL=1 FD6288_STARTUP_RESCUE_RETAIN=1 FD6288_STARTUP_RESCUE_BRIDGE=1 FD6288_STARTUP_ACQ_LOG=2 FD6288_STARTUP_S4E=1 FD6288_STARTUP_LOSS_LOG=1
```

单target，新开关默认0，必须配合S4E。
HEX：`AM32_CUSTOM_FD6288_G431_S4E_LOSS_ACQ300_MANUAL_STOP.hex`。
SHA256：`afc035876bd5bee3d8bb6136f9616723cfaed503154f12851e942f6bf061964a`。
ELF：`obj-startup-S4E-loss/AM32_CUSTOM_FD6288_G431_2.20.elf`。
36016数据字节，仅0x08001000..0x0800F7FF，无BL/F800；reset=0x08006359。

实际C记录器ASan/UBSan、新旧模式、盲试探不触发、第二边沿后
22.5ms窗口覆盖、后续不覆盖、成功接管冻结、新启动重置、C→JS、
非法元数据拒绝、只读mock、目标宏、镜像边界及ARM日志检查通过。
原newlib stub/RWX警告保留。软件验证不是启动或堵转实机验收。
关闭新开关重建的S4E HEX与原已测版本逐字节一致。

已按用户授权只写主固件，校验36016 B通过，BL/F800不在写入范围，
设置RAM前后相同；重启variant18、trial=0、armed=1、输入/CCR均零。
日志：`../../hardware-backups/startup-S4E-LOSS-20261018/flash-1792335588977.json`。
同设置短测，正常起转后主动回零、异常立即回零，
记录失败序号和抖动/短暂反转，保持供电读取。不手拨桨或延长异常
通电。回滚使用S4E（variant17）的原HEX，不需更改BL/F800。
