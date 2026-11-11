# NATIVE-SINE CLOCKFIX已烧录，2026-11-08

用户明确“刷吧，我已经睡醒了”。旧INHERIT-TIMING配对ELF检查油门／运行／PWM／驱动全零，确认设置仍为前一轮正弦配置后，仅刷CLOCKFIX主程序并reset run成功；未写设置页、未发送油门、跳过固件读回校验。OpenOCD报告写入31536B。

首次0.5秒后检查遇到启动输出尚未归零，未重复擦写／复位；启动结束后补读确认输入、运行、Coast、duty、GPIO及CCR1..4全零。

运行态DWT确认：CTRL=0x40000001，DEMCR=0x01000000，CYCCNT两读3075748250→3076087266，delta339016，fd_native_clock_ok=1。计数器使能、实际推进及启动自检均通过。这不是完整固件读回校验；尚未做本轮电机试验。

当前镜像：test-firmware/fd6288-NATIVE-SINE-CLOCKFIX-20261108/AM32_CUSTOM_FD6288_G431_2.20.hex
当前匹配ELF：同目录AM32_CUSTOM_FD6288_G431_2.20.elf。
主HEX SHA256：d7d27695382e895dd12aa968dd1ca93fb92afcef2fb82dadd32e109bc72882f9。

保持150%、900KV、原进角、正弦开／range15／power6。整次3秒上限，六个真实反馈后最多再1秒，先到者关输出。读取器在 /private/tmp/fd6288-native-sine-20261108/tools/scripts/fd6288_native_sine_capture.cjs，归零停稳后读取新ELF。
