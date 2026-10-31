# 2026-09-20 带载启动检查：只读备份

用户确认油门为 0、电机完全停转。负载已安装，但本次未要求起转。
使用 ST-Link / OpenOCD 400 kHz SWD 读取；未执行 halt、reset、resume、
program、erase 或设置写入。禁用了默认 examine-end 寄存器修改和 GDB 服务。

## 文件与固件身份

- `flash64k.bin`：`0x08000000..0x0800FFFF` 完整回读，包含 BL、主程序、设置。
- `ram-bootloader-mode.bin`：`0x20000000..0x20001FFF` 快照。读出时
  `VTOR=0`、TIM1 寄存器全零、主控状态 running；与引导/地面站模式一致。
  **不能把此 RAM 快照按主程序 ELF 解释成当前电机控制状态**，残留值无效。
- 完整 Flash SHA-256：
  `7b02978f7a774f2183adc847fef9ff557c51ddb1704704c096fa8a091154eeee`。
- 主区与候选 BIN 的差异仅在 HEX 未定义的空洞：
  `0x080011D8..0x080011FF` 和 `0x08007FB4..0x0800F7DF`。
  板上为擦除值 FF，BIN 填充值为 00；不能仅凭整个 BIN 哈希不一致判断刷错。
  实际 HEX 记录逐字节比对通过，候选为 commit `2c1b5fa`。

## 设置异常：来自 Flash，不是截图推测

设置基址仍为 `0x0800F800`，版本 3，固件版本 2.20。
偏移 5..12 的原始字节为 `55 6E 6B 6E 6F 77 6E 20`，即 ASCII `Unknown `。
相比 2026-09-15 发布设置：

| 偏移 | 字段 | 发布备份 | 本次实读 |
| --- | --- | ---: | ---: |
| 5 | max_ramp | 160 | 85 |
| 6 | minimum_duty_cycle | 1 | 110 |
| 7 | disable_stick_calibration | 0 | 107 |
| 8 | absolute_voltage_cutoff | 10 | 110 |
| 9 | current_P | 100 | 111 |
| 10 | current_I | 0 | 119 |
| 11 | current_D | 100 | 110 |
| 12 | active_brake_power | 0 | 32 |
| 22 | stuck_rotor_protection | 1 | 0 |
| 24 | pwm_frequency | 24 | 48 |
| 47 | auto_advance | 1 | 255 |

其他 0..47 字节与该发布设置一致。方向、3D、正弦启动均为 0；互补 PWM=1；
startup_power=150，KV 编码 22（900 KV），极数 14，advance_level=34。
电流限制=0、低压保护=0、温度限制=255，不能依赖这些关闭的保护兜底。

代码解释（尚未在运行主程序中读回确认）：

- minimum_duty_cycle=110 超出当前固件 1..50 的合法范围，先按 0 处理，
  再叠加 target 固定死区补偿 200；并不是直接输出网页显示的 55% duty。
- pwm_frequency=48 在 RAM 加载时限制为 24，不自动改写 Flash。
- auto_advance=255 在 C 的非零判断中为 true，而网页布尔显示只接受 1；
  因此截图的未勾选不能证明运行的是固定 22.5 度进角。
- 网页读写字段偏移与本固件一致，不是 F800/FC00 地址混淆的证据。
- 数值异常的来源尚未查明，不能归因于用户操作或候选固件；也不能据此
  声称已确定带载启动/转向异常的全部根因。

网页源码核对：`am32-firmware/am32-configurator` commit
`ac2e2cfc608f2de3bbb79bbf06d75be1a77920fc` 的 `src/eeprom.ts`、
`components/SettingField.vue`。本地解释见 `Src/main.c:loadEEpromSettings()`。

## 后续门槛

暂缓带载测试，等待用户补充最近的升级/设置操作历史。
先明确并获准恢复已知设置基线（保留用户当前反转关闭、堵转保护关闭），
逐字段核验；退出地面站并正常重新上电后，只读核实主程序的实际参数。
不自动写设置、不发送默认配置、不改变 BL 或启动判定逻辑。
配置一致之后才安排短时启动采样；异常立即由用户回零，不保持故障等抓取。

## 同日修复准备（尚未写入）

用户同意修复，并说明昨天用仿真器刷入过 F800 HEX。随后补充：最新候选
约 80% 正常，约 20% 正向卡顿、约 2.7 A；失败不再观察到反转。此为用户
观察，未与本次 RAM 记录建立逐次对应，不能标成同步抓获的故障波形。

本地发布 SETTINGS HEX SHA-256 与 GitHub Release 资产摘要一致
（`7beb53f5d2ccb71b6ba4c8424a18113bd827d02f78d11d833d5f15a2730454a9`）。
逐记录检查：地址仅为 F800..FFFF，校验和有效，与已知正常 SETTINGS BIN
逐字节一致，没有 `Unknown ` 异常字段。文件带有一条 type-05 起始地址记录
F800（不写数据）；**设置页不是可执行程序，不应选择从此地址运行**。
本次新生成设置 HEX 不包含执行入口记录。用户实际选取的文件尚未取得，
不能断定异常来源，也没有证据证明是发布文件或仿真器造成。

重新只读连接后，`VTOR=0x08001000`，GPIOA PA8..PA12 与 GPIOF PF0 均为
低电平 GPIO 输出，TIM1 CCR1..4=0，输入和 running 均为 0。
`ram-stopped-pre-repair.bin` 是这时的主程序 RAM 快照，可按候选 ELF 解释。
`flash64k-pre-repair.bin` 是最新完整备份，与第一次 Flash 回读完全相同。

`tools/scripts/prepare_fd6288_settings_repair.cjs` 离线生成修复页，不连接硬件。
它核验发布基线 SHA、HEX/BIN 一致性、当前异常特征和方向/保护开关，
只更改偏移 5..12、24、47，其余 2038 字节保留。生成文件均采用独占创建，
不会覆盖之前备份。

- 实际待写：`settings-repaired-20260920.bin` / `.hex`，对应 F800 的 2 KiB 页。
- BIN SHA-256：`852293fdb1c158518b9b9b4575dad3fbf74b2e7205fee5f73b44fb461b725e08`。
- HEX SHA-256：`5558d71199ac21f0a33e6efa854e27bb3d965666912e3e7c946d591583b50964`。
- 逐字段清单及原始哈希见同名前缀 `.json`。
- 方向=0、堵转保护=0、启动功率=150、其余限制开关保持不变；自动进角恢复
  合法值 1（之前非法 255 在固件里也按开启处理），PWM 明确存成 24。

由于用户在修复准备中报告刚刚又做过起转观察，已重新询问停止状态和
地面站断开状态。在收到确认前，没有 halt/reset/Flash 写入。

## 已完成设置修复及重启核验

用户随后明确确认停转并断开地面站，授权修复。又补充当天使用过 RF 的
AM32 前向编程；该线索尚未确认为异常来源，需取得具体工具/固件版本。

1. 重新只读备份 `flash64k-write-preflight.bin`；它与准备修复时的完整
   Flash 相同。即刻复核 running/newinput/adjusted_input/input 均为 0，
   PA8..PA12 与 PF0 为 GPIO 低电平，TIM1 CCR1..3=0。
2. 首次在已运行过主程序的环境中 halt，再写设置时，RAM Flash 算法被
   看门狗复位打断。`RCC_CSR=0x2C000000` 含 IWDGRSTF，PC 返回 BL，
   工具报 block write failed。没有将该次操作记为成功或直接 resume。
   `flash64k-interrupted-write.bin` 证明 BL/主程序未变，设置页全为 FF。
3. 按 `reset init` 流程停在主程序/看门狗启动之前，再只擦除
   `0x0800F800` 的 `0x800` 字节、写入完整修复页。`verify_image` 成功。
   `flash64k-post-repair-halted.bin` 完整回读证明 BL+主程序逐字节未变，
   全 Flash 只有设置偏移 5..12、24、47 这 10 字节与写入前不同。
4. 执行正常 `reset run`；VTOR 回到 `0x08001000`。运行态 RAM 另存为
   `ram-post-repair-running.bin`，完整 Flash 另存为
   `flash64k-post-repair-running.bin`，与停核校验后的 Flash 完全一致。

最终完整 Flash SHA-256：
`65b582318e42f338fbe736b63d246003383b160f499f09bb47f33efbd2b047d7`。

| 主程序实际值 | 修复前 | 修复后 |
| --- | ---: | ---: |
| minimum_duty_cycle（内部 0..2000 单位） | 200 | 210 |
| min_startup_duty（同上） | 350 | 360 |
| startup_max_duty_cycle（同上） | 600 | 610 |
| TIMER1_MAX_ARR / TIM1 ARR | 6665 | 6665 |
| RAM pwm_frequency | 24 | 24 |
| RAM auto_advance | 255 | 1 |
| dir_reversed | 0 | 0 |
| stuck_rotor_protection | 0 | 0 |

重启后 forward=1，running/input/newinput=0，PA8..PA12 与 PF0 仍为 GPIO
低电平，TIM1 CCR1..4=0，DTG 仍为 0xCE。这里只核验设置和停止状态，
没有进行修复后的带载启动试验，也没有宣称解决剩余约 20% 起转失败。

SWD probe 报告芯片物理 Flash 为 128 KiB；本项目仍使用前 64 KiB 的
既定 BL/主程序/F800 设置布局，不能据此改刷 0x4000/0x1F800 布局。
本次没有擦写后半 Flash。

RF 初步只读对照的官方源码：
- 固件 `33be7667142fa8cdeb0e0505fd2fdcb9e2aa26f8`，
  `src/main/sensors/esc_sensor.c`。
- RF2 Lua `da7738cb4ec681f768d8b11873da9161a1d3fa98`，
  `src/SCRIPTS/RF2/MSP/mspEscAm32.lua`。

这些版本按字段转发设置；未据此定位到制造本次 `Unknown ` 字节的路径。
不等于证明用户正在使用的版本兼容或排除其他 RF 工具；先暂停通过该入口
保存参数，取得实际版本/脚本后再做离线对照，不用带载复现设置污染。
