# SINE-ONLY已烧录，2026-11-08

用户明确要求“只运行正弦试试”，观察正弦本身是否带得起来。已用当前CLOCKFIX配对ELF确认油门／运行／PWM／引脚全零，仅刷仅正弦主程序并reset run；不写设置页，不发送油门，按既有要求跳过固件读回校验。OpenOCD报告31128B。

重启后零输入／零输出，DWT_CTRL=0x40000001，CYCCNT两读推进347308cycles，fd_native_clock_ok=1。当前还没有用户实机效果反馈。

当前配对ELF：test-firmware/fd6288-SINE-ONLY-20261108/AM32_CUSTOM_FD6288_G431_2.20.elf。
主HEX SHA256：f007af9d214a5fd91824d98d1b1805c5454caac40dab30541afe570634fb20b4。
读取器 /private/tmp/fd6288-native-sine-20261108/tools/scripts/fd6288_native_sine_capture.cjs，归零停稳后只读；它检查main身份，不能混用交接版ELF。

输出只正弦，不进入六步／反馈／超时推进。原出力power6和原生油门到角度速度映射保留，高油门仍受每电角度最短57us限制。整次约3秒关输出，需回零才能再试。只需肉眼分清一开始平顺／先咔后顺／一直咔；速度慢和最终自动停本身不算失败。摘要sixstep_at应未发生、edges应0，均为本版预期。
