# S2：减缓启动加速，偏晚边沿时保持速度；仍为台架版

状态：2026-09-21 已刷入主 HEX，单次 verify_image 通过，停止态运行检查通过；
**已完成一轮 10 次电机实测，仅第 10 次完成闭环接管，不可飞行**。未刷新 BL/F800。
详见 [2026-09-22 实测记录](../../hardware-backups/startup-S2-20260922/README.md)。
板上当前为 S2。S1 数据见
[首次失败记录](../../hardware-backups/startup-S1-20260921/README.md)。

## 依据与边界

S1 已执行计划定位/加速，用户观察缓慢转动；53 次开环步进仅产生 12 个
开环 ADC 接受边沿（另一个在定位期）。最高资格连续数为 2，没有接管。
131/138/144 ms 三个边沿的扇区位置由约 59%→86%→92%，最后超出窗口；
与此同时 S1 不论边沿位置如何，仍每步把周期乘 15/16。

据此优先检验“要求的加速与实际转子跟随不匹配”，但不是已证明唯一根因。
采样门限/模拟信号、定位未充分稳定、低速扭矩不足仍可能参与；不能只把
6 次确认降成 2 次，也不能据此无限提高功率或延长无反馈通电。

[ST UM3468 §2.2](https://www.st.com/resource/en/user_manual/um3468-stm32-motor-control-sdk-6step-startup-configuration-stmicroelectronics.pdf)
建议带惯量负载在启动阶段采用较慢加速段；
[ST 调参说明](https://wiki.st.com/stm32mcu/wiki/STM32MotorControl%3A6-step_-_Optimization_and_troubleshooting_of_sensor-less_firmware_parameters)
也说明检测门限会造成边沿提早/滞后。文献支持调试方向，**不验证下列
电机专用参数或说明 ADC 问题已被排除**。

## 相对 S1 的修改

1. 开环每步周期由乘 15/16 改为 31/32：每步缩短 6.25%→3.125%。
   仍从 12 ms 开始、最低 4 ms。这是减缓节奏，不宣称是恒机械加速度。
2. 当相邻两个或更多边沿已通过原有位置和周期检查，且当前边沿落在扇区
   后 1/4，后续暂保持当前周期。每次请求保持 6 步；新的合格偏晚边沿可
   再请求保持，但无论如何不能越过 300 ms 启动截止。保持不等于接管。
3. 六边沿接管门槛、12.5%..87.5% 位置窗口、75%..125% 周期检查不变。
   ADC 鲜样本/ON 窗口/消隐/前态/迟滞/确认不变；不接受错方向或静态后侧。
4. 功率不增加：定位仍请求 360、开环仍请求 400；原有斜率和保护上限保留。
   30 ms 定位、物理初始矢量和方向规则不改，便于归因。
5. **启动未接管：最多 300 ms。成功接管：从接管时刻再观察最多 1 秒。**
   部分合格或等待接管没有延期资格。总窗口小于约 1.3 秒；回零可提前结束。
6. 延长观察期后，首个原生缺失反馈超时（计时器 >45000 个 0.5 µs tick）
   即触发额外的台架停机，不继续强制步进重试。原生堵转开关/阈值/锁定
   函数保留，但该台架停止可能先于原生保护，所以**不能据此证明原生堵转
   故障触发已验证**；可先观察保护 On 是否影响正常起转。
7. 新 variant=5。记录每个边沿的资格结果（太早、太晚、周期不一致等）与
   扇区位置百分比；最近 16 次摘要新增最高连续合格数。首次失败详细 trace
   继续自动保留，日志截止延至 2 秒，仅用于覆盖最长 1.3 秒试次，不控制电机。

BL、F800、地面站入口、EEPROM 布局、正常 ADC 标定、固定死区未改。
仍沿用台架限制：不支持旋转中的正油门 Coast 接管，不可当稳定飞行版。
接管后普通油门映射仍工作；不要因有自动截止就在首次测试推到满油门。
所有上述数字均待硬件验证；“合格边沿”是软件判定，不是独立机械位置测量。

## 主文件

- `AM32_CUSTOM_FD6288_G431_S2_BENCH_ACQ300_OBS1000.hex`
- SHA-256：`a9418d8afe4aab71b50fa03ef9ec3683b93786f678f7624ee261db4fd7897525`
- 主区 `0x08001000..0x0800F7FF`，无 BL/F800 数据记录。
- 配对 ELF：`obj-startup-sequence-S2/AM32_CUSTOM_FD6288_G431_2.20.elf`。
- HEX 有效数据 34356 B，RAM 28448 B；链接现存系统调用桩/RWX 警告不变。

```sh
make -j1 CUSTOM_FD6288_G431 OBJ=obj-startup-sequence-S2 ARM_SDK_PREFIX=arm-none-eabi- FD6288_STARTUP_TRACE=1 FD6288_STARTUP_TIMING_FIX=1 FD6288_STARTUP_SEQUENCE=2
node tests/check_fd6288_image.cjs obj-startup-sequence-S2/AM32_CUSTOM_FD6288_G431_2.20.bin obj-startup-sequence-S2/AM32_CUSTOM_FD6288_G431_2.20.hex
node tests/run_fd6288_host_tests.cjs
node tests/fd6288_s1_replay.cjs
node tests/fd6288_capture_tests.cjs obj-startup-sequence-S2/AM32_CUSTOM_FD6288_G431_2.20.elf 5
```

只构建本板型。S1=`FD6288_STARTUP_SEQUENCE=1`；默认 0 不启用序列实验。
不同编译选项使用不同 OBJ，避免 Make 漏掉选项变化。

## 验证与下一轮

S2 函数回归 ASan/UBSan 12804 条断言通过（包含重复截止检查，并非同数量
独立场景），A/B/P0/P1/S1 回归通过。真实 S1 事件回放复现 53 步、13 边沿、
最高连续 2、无接管；**没有把 S1 波形当成修改后 S2 的物理仿真**。
HEX 地址/校验和/向量/BIN 一致性以及采集模拟 RPC 测试通过。
S1 重编译 HEX 与板上 S1 发布文件逐字节一致，便于回退。

下一轮刷主区后，先短时卸载确认停止和方向，再在固定、安全隔离条件下
按此前同一油门请求做少量冷起转，之间回零并完全停稳。未接管约 0.3 秒
停止；接管可能持续约 1.1–1.3 秒，不再把“超过 300 ms”当作失控。
实际卡顿、异常电流或温升立即回零；不以手拨或手堵转验证。
如果改设置需重启，先读出 RAM。按用户要求保留精简刷新，不重新全片备份。

停止态读取（这是后续操作说明，本轮未执行）：

```sh
node tools/scripts/fd6288_startup_capture.cjs dump obj-startup-sequence-S2/AM32_CUSTOM_FD6288_G431_2.20.elf NEW-S2-TRACE.bin --stopped --quick-dump
```
