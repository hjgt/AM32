# SINE-CENTERED已烧录（2026-11-08）

用户明确授权“刷吧”。原SINE-ONLY匹配ELF读取输入/运行/PWM/GPIO全零后，主程序写入31432B、reset run成功。HEX SHA256 60a4673fa306a65899ca1bb8e7c51c657f3b0d6cbf3d58142a3292eee6be920f。保留现有参数，跳过固件读回校验，未发送油门。

启动结束后确认输出关闭；DWT_CTRL=0x40000001，CYCCNT两读推进289420，clock自检1。仍需电机实测。只运行正弦，不切六步，power6、原进角，约3秒上限或归零提前停止。

后续读取必须用test-firmware/fd6288-SINE-CENTERED-20261108/AM32_CUSTOM_FD6288_G431_2.20.elf；读取器在独立分支工作区tools/scripts/fd6288_native_sine_capture.cjs，可读80B停机前快照。固件包verification.json的flashed=false是构建时状态，本次实际烧录状态见本目录flash.json。
