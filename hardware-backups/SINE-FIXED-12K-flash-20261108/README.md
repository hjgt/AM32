# SINE-FIXED-12K已烧录（2026-11-08）

最新实测：[trial7结果](RESULT-no-change-trial7-20261108.md)。12K与相线约4～5us差窗生效，249个可用引脚样本全部吻合；93次传感器更新，过滤电流0.04→最高0.10A，母线最低7.86V，无明显机械改善。本次只读，板上保持本版。以下为此前烧录记录。

用户已明确授权烧录，随后回复“已接好，再试烧录”。此前无法打开ST-Link的尝试未触碰目标，保留flash-attempt1-service-unavailable.json。重新连接100kHz ST-Link成功，目标约3.259V，确认旧24K固定版输入/运行/PWM/GPIO均零后，12K主程序写入34936B、reset run成功。

HEX SHA256 071926e60a7f080b79e8de7580547010caeb06d4bd2aceee441edfea8228227e。保留原EEPROM页、跳过固件读回校验、未发送油门。启动后确认输出关闭，DWT_CTRL=0x40000001、CYCCNT推进370657、自检1。

另只读核对实际运行配置：RAM pwm_frequency=12，TIM1 ARR=13330，CCR1..4全零，见carrier-postflash.json。因此12K载波配置已在板上生效，尚无本版电机实测。

本版固定电角度150/269/29、power6、2.3us死区、最多100ms，不连续旋转、不自动重复。保留3096B引脚记录和80B输出快照，新增40B约1ms采样节拍的电流/母线峰谷摘要。用户短给一次油门、归零后保持供电读取。

当前读取ELF必须改用test-firmware/fd6288-SINE-FIXED-12K-20261108/AM32_CUSTOM_FD6288_G431_2.20.elf；独立分支tools/scripts/fd6288_native_sine_capture.cjs已支持传感器摘要与12K引脚对照。包verification.json的未烧录状态是构建时历史状态，实际烧录以flash.json为准。48K最终目标不变，本版仅作诊断。
