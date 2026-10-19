# S4G-SEED：首个合格过零后500us换相等待对照

2026-10-19，variant20，已生成未烧录。基线S4F-PHASE variant19。
仅台架实验，尚无实机改善/堵转保护可用/可飞行结论。

## 单变量

每段连续反馈的第一个合格过零后，等待固定500us再换相，不再立即换相。
超时清零连续计数后，再次出现的首边沿也采用该等待。不是每个强制步骤
增加500us，也不是额外试探一次。此时没有可信过零周期，绝不把换相至
首边沿时间冒充实测电周期；500us是实验值，不是测得的最佳延迟。
由现有服务执行，实际换相有周期量化及执行开销，不承诺精确500us。

其他保持S4F：第2～5边沿按真实相邻间隔和提前角计算，最大3ms；
第六边沿接管及正常闭环不变。完全无反馈的22.5ms等待、一次六段8ms
预算不变，不增加重试次数。功率360/400、过零预备/迟滞/确认、消隐、
原生堵转开关/阈值均不改。300ms获取截止仍优先，延迟等待不能延长它。
成功接管后没有1s自动停机；正常也须主动回零，异常立即回零。
时机变化可能间接改变超时次数和堵转触发率，不能宣称保护已验证可用。

## 依据与不确定性

S4F #8只有一个接受边沿，从未执行第2～5边沿延迟。首边沿46.875ms，
随后step4全部220个保留点为unarmed，directed_margin=+26..+510，
没有负侧预备。ADC换相路径已检查，会清空预备/确认/报告状态并消隐，
未发现漏复位；不据此取消预备条件。
新增等待用来对照首边沿后立即换相的影响，不能保证改善，也可能倒退。
记录器沿用首次反馈后超时冻结，失败/成功分类仍分别保存。

## 构建与文件

```sh
make -B -j1 CUSTOM_FD6288_G431 OBJ=obj-startup-S4G-seed ARM_SDK_PREFIX=arm-none-eabi- FD6288_STARTUP_TRACE=1 FD6288_STARTUP_TIMING_FIX=1 FD6288_STARTUP_SEQUENCE=3 FD6288_STARTUP_PAIR_TRACE=1 FD6288_STARTUP_WAVE_TRACE=1 FD6288_ZCD_ACTUAL_DEADTIME=1 FD6288_STARTUP_S4=1 FD6288_STARTUP_HANDOFF_REAL=1 FD6288_STARTUP_RESCUE_RETAIN=1 FD6288_STARTUP_RESCUE_BRIDGE=1 FD6288_STARTUP_ACQ_LOG=2 FD6288_STARTUP_S4E=1 FD6288_STARTUP_LOSS_LOG=1 FD6288_STARTUP_PHASE_DELAY=1 FD6288_STARTUP_SEED_DELAY=1
```

HEX：`AM32_CUSTOM_FD6288_G431_S4G_SEED_ACQ300_MANUAL_STOP.hex`。
SHA256：`214dd2c49cf71c8c8c874e026b41b6d839ce0235dad8083bae3ba0d81ea39758`。
ELF：`obj-startup-S4G-seed/AM32_CUSTOM_FD6288_G431_2.20.elf`。
36044数据字节，全部0x08001000..0x0800F7FF，不含BL/F800设置。
RAM26368 B；SP0x20008000，reset0x08006375。
默认新开关0，必须搭配S4F基线。只编译本板target。
关闭新开关重建HEX与原S4F variant19逐字节相同。

## 软件验证及短测

实际ADC/适配器variant20共744断言及旧变体回归通过；首边沿和后续
pending期间回零取消、故障/300ms优先、超时清零、重复边沿拒绝、
所有提前角输入/计数器回绕的策略ASan/UBSan测试通过。
波形C→JS/只读mock抓取、非法配置、镜像范围、ARM日志检查通过。
原newlib stub/RWX链接警告保留。软件测试不等于机械模型或实机验收。

同设置先3～5次，记下失败序号、反向抽动、起转延迟及电流。异常立即
回零，不用高油门硬顶；停稳后保持供电读数据。正常也主动回零。
若没有优势，回滚S4F原HEX，不改变BL/F800；不要用少量样本宣称成功率。
回滚：`../fd6288-startup-S4F-PHASE-20261019/AM32_CUSTOM_FD6288_G431_S4F_PHASE_ACQ300_MANUAL_STOP.hex`。
本轮未操作硬件，板上仍variant19。
