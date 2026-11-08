# NATIVE-SINE 保留原进角版已烧录，2026-11-08

用户明确要求烧录。用当前 EARLY-REKICK 配对 ELF 检查输入／运行／PWM／驱动引脚均为零，确认 Flash 设置前48字节仍与刚备份原页一致后，reset init、写入主程序＋配置合并HEX、reset run 均成功。OpenOCD报告写入33136 B。按用户要求跳过烧录后读回校验，没有发送油门。

固件：test-firmware/fd6288-NATIVE-SINE-INHERIT-TIMING-20261108/NATIVE-SINE-COMBINED.hex
SHA256：ec4c8c583a5df747608f938dd4801acb439364efe8ea62b01c037215a1bdf104
当前读取 ELF：test-firmware/fd6288-NATIVE-SINE-INHERIT-TIMING-20261108/AM32_CUSTOM_FD6288_G431_2.20.elf
读取器在独立分支工作区 /private/tmp/fd6288-native-sine-20261108/tools/scripts/fd6288_native_sine_capture.cjs，归零停稳后使用 --stopped，只读。

150%、900KV、正弦开启、range15、power6；进角继承原参数（原页编码18、自动进角0）。实际设置仅19/40/45三个字节改变。完整原页和恢复HEX已备份于 hardware-backups/NATIVE-SINE-parameters-before-20261108/。

有界台架版：整次最多3秒；六次真实反馈后最多再观察1秒，先到者关输出；归零立即结束，正油门不自动重启。尚无本版实机起转结果。不要用旧EARLY-REKICK ELF读取新RAM。
