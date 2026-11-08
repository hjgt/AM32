# SINE-ONLY几乎不动：输出适配审计（2026-11-08）

用户观察：转子只极轻微动一下，电流看不出上涨。此次不把它归因于电机跟不上开环，先核对程序到功率桥的输出。

## 实际读取

仅只读、未halt/reset/烧录/发送油门。最新摘要trial8，正弦命令推进18799次，1400.434ms后解码油门归零停止；DWT计数和自检均正常。没有进入六步，是SINE-ONLY设计。该摘要只说明软件在推进，不能说明电机转动，更不能据此推断三相输出正确。

- [摘要原始JSON](../hardware-backups/SINE-ONLY-flash-20261108/capture-no-motion-20261108.json)
- [停止态TIM1/GPIO/运行参数](../hardware-backups/SINE-ONLY-flash-20261108/no-motion-stopped-registers-20261108.json)
- [脉宽计算结果](../hardware-backups/SINE-ONLY-flash-20261108/deadtime-audit-20261108.json)

## 已确认的缺陷

停止态TIM1：CR1=0x81（CKD DIV1）、PSC=0、ARR=6665、CCER=0x1555、BDTR=0x0200a0ce；实际时钟160MHz，24kHz边沿对齐PWM。DTG=0xce按编码得到368ticks=2.3us死区，不是旧gate_drive_offset=70所代表的0.4375us。

现有原生正弦公式，TIMER1_MAX_ARR=6665、offset70、power6，产生CCR139..859，对应高侧命令宽度0.869..5.369us。其中每相137/360个角度位置不超过368ticks。STM32高级定时器会抑制短于死区的脉冲，见[ST RM0440高级定时器dead-time insertion部分](https://www.st.com/resource/en/reference_manual/dm00355726-stm32g4-series-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf)。因此本板原生正弦输出发生窄脉冲裁剪；六步专用的固定死区占空比补偿不能自动修正此独立正弦公式。

停止态CCR为0、GPIO关断正常，不能当成试验中完全无输出的证据。窄脉冲缺陷由本板死区配置与实际公式共同确定，不是示波器对有载波形的测量。它能使此前“软件在转角度，所以是电机拖不起来”的判断失去依据；仍不能单凭此计算断定电流或机械不动的全部原因。

## 已完成但未烧录的修正

独立分支codex/fd6288-native-sine-20261108，新增默认关闭开关FD6288_SINE_CENTERED_PWM，仅准用于SINE-ONLY。将三路比较值共同平移到周期中段，三相比较值之差逐点完全不变；保持正弦power6、startup150%、原进角、速度映射、24kHz、2.3us死区和3秒边界。定时器仍边沿对齐，不改计数模式。UDIS短暂阻止预装载转移，三路写完恢复原CR1；不清计数器、不强制更新。

正反两个方向的全部360角度，实际C代码与实际GPIO模式函数离线执行（ASan/UBSan），修正输出3020..3646ticks，高低间隔最小18.875us；每对CCR差值与旧公式完全一致。时基/死区异常拒绝输出。原SINE-ONLY及ADC交接回归通过；关闭新开关构建HEX与板上SINE-ONLY字节完全相同。ELF/HEX/BIN一致。详见[构建包](../test-firmware/fd6288-SINE-CENTERED-20261108/README.md)。

新增80B最后驱动状态快照，只在停机前读一次TIM1/GPIO/相位。用于下一次实测核对输出配置，不在ADC或角度推进路径记录波形。它仍不是有载电压、电流测量。

**以下为修正构建时结论；随后已刷SINE-CENTERED，用户实测无改善。** 最新读取见[trial4结果](../hardware-backups/SINE-CENTERED-flash-20261108/RESULT-no-change-trial4-20261108.md)。本次不能把窄脉冲裁剪视为不起转主要原因。 暂不重复当前旧版电机试验，也不通过加功率掩盖输出问题。原参数完整备份与试验前主线均保留。
