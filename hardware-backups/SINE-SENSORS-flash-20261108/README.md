# SINE-SENSORS已烧录（2026-11-08）

用户明确要求编译并烧录，已说明本次先烧传感器更新修复，保留3秒正弦/power6。重新构建HEX/ELF/BIN与已验证包哈希一致。第一次旧SWD会话读内存失败、未写入，保留flash-attempt1-swd-read-failed.json；关闭失效OpenOCD后常规100kHz重新连接成功。

确认SINE-CENTERED匹配ELF输入/运行/PWM/GPIO全零后，SINE-SENSORS主程序写入31472B、reset run成功；保留设置页、跳过固件读回校验、未发送油门。HEX SHA256 8039cc2cf9a10b315a8e528ef5f80c4d39dfed09d5785289992abd70b7380e39。启动后输出关闭、DWT_CTRL=0x40000001，CYCCNT推进363177、自检1。

只读传感器状态：stepper_sine=1、计数器20和18（均回到21tick周期内），battery_voltage=819（8.19V），原始母线1682、电流10，actual_current=3。这确认正弦状态下的传感器调度已恢复，尚无有载电流/电机动作效果结论。

仍全程正弦、不交六步、power6、原进角和速度映射、最多3秒或回零提前停。没有实施固定相角诊断。当前读取必须改用test-firmware/fd6288-SINE-SENSORS-20261108/AM32_CUSTOM_FD6288_G431_2.20.elf，独立分支读取器支持80B最后驱动状态快照。验证包flashed=false是构建时快照，实际烧录状态以本目录flash.json为准。
