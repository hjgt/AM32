# S4G-NOEDGE：完全无过零时保留300ms全程粗采样

2026-10-19，variant21，已生成未烧录。控制策略仍S4G-SEED。
本版只改诊断，不是启动问题已修复或可飞行版。

## 原因与改动

上轮#14没有任何接受边沿，记录器只留下273.912～299.972ms，不能
判断早期六段8ms牵引失败在哪一步。用户观察卡两下、约转90度并不能
证明继续拉一定接管。本轮不凭尾部波形改强拉参数，不延长300ms截止。

首次接受前：每至少1.2ms保留一点，到300ms截止，256槽容纳整段粗采样。
首次接受后：永久切回原至少100us环形采样；仍在第一次反馈后超时或
接管时冻结。无额外RAM、无数组清空、事件/计数/精确首边沿字段不变。
新trial重新使用粗采样；无边沿时晚于300ms的服务不覆盖已保存前缀。
全程是覆盖时间范围，不是连续波形；亚毫秒变化可能漏掉，ADC不是示波器。
有边沿的trial高频环仍可能覆盖早期粗采样，不能同时保证两种全程历史。
全局reserved[2]记录192000 ticks，原100us和事件触发字段不变。

控制代码、功率、开环节奏/预算、首边沿500us、第2～5边沿延迟、正常
闭环、检测门限及保护均未改。记录开销变化可能影响实机时序，不能宣称
绝对零扰动。接管后仍不会在1s自动停机，测试结束需主动回零。

## 文件与验证

HEX：`AM32_CUSTOM_FD6288_G431_S4G_NOEDGE_ACQ300_MANUAL_STOP.hex`。
SHA256：`b1b0aa219b6f6c298ac2b73fcbaa4dd0f975085924b8333166b7730a9e16d25c`。
ELF：`obj-startup-S4G-noedge/AM32_CUSTOM_FD6288_G431_2.20.elf`。
36116数据字节，全部0x08001000..0x0800F7FF，不含BL/F800。
RAM26368 B；SP0x20008000，reset0x080063BD。

```sh
make -B -j1 CUSTOM_FD6288_G431 OBJ=obj-startup-S4G-noedge ARM_SDK_PREFIX=arm-none-eabi- FD6288_STARTUP_TRACE=1 FD6288_STARTUP_TIMING_FIX=1 FD6288_STARTUP_SEQUENCE=3 FD6288_STARTUP_PAIR_TRACE=1 FD6288_STARTUP_WAVE_TRACE=1 FD6288_ZCD_ACTUAL_DEADTIME=1 FD6288_STARTUP_S4=1 FD6288_STARTUP_HANDOFF_REAL=1 FD6288_STARTUP_RESCUE_RETAIN=1 FD6288_STARTUP_RESCUE_BRIDGE=1 FD6288_STARTUP_ACQ_LOG=2 FD6288_STARTUP_S4E=1 FD6288_STARTUP_LOSS_LOG=1 FD6288_STARTUP_PHASE_DELAY=1 FD6288_STARTUP_SEED_DELAY=1 FD6288_STARTUP_NOEDGE_LOG=1
```

新开关默认0，要求S4G基线。只编译本板target。
关闭新开关重建HEX与原S4G-SEED逐字节相同。
实际C记录器ASan/UBSan检查：无边沿300ms跨DWT回绕250点不覆盖、
迟到服务不覆盖、重开trial、首边沿后100us切换、原冻结/分类保持；
新增无边沿C→JS夹具、非法元数据拒绝、旧variant20回归、只读抓取mock、
目标宏、镜像边界及ARM日志检查通过。原newlib stub/RWX警告保留。

## 测试与回滚

同设置分组测试；出现失败即回零、停稳并保持供电读取，尤其记录是否
卡顿移动/反向抽动。异常不要增加油门硬顶，正常也主动回零。若前5次
均正常可先报告，不需要为了造失败长时间重复带载。
回滚S4G-SEED原HEX即可，BL/F800不动。本轮未操作硬件，板上仍variant20。
