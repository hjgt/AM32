# SINE-FIXED-PROBE已烧录（2026-11-08）

最新结果：[trial3引脚对照](RESULT-fixed-trial3-20261108.md)：244可用样本全部吻合，16条A/B高侧不同、2条AH+BL同时为高。MCU端已输出差异，下一步同版测相线差动电压。以下为此前烧录记录。

用户明确“烧录吧”。使用SINE-SENSORS匹配ELF只读确认输入/运行/PWM/GPIO全零后，固定相角诊断主程序写入34752B、reset run成功。HEX SHA256 5ce35620c428ad450b9b265dc114c9af0dbf281aa96b8bf07541ed8640032b3f。保留参数页、跳过固件读回校验，未发送油门。

启动输出结束后确认输出关闭；DWT_CTRL=0x40000001，CYCCNT两读推进355903，自检1。尚无本版电机实测或引脚采样结果。

本版固定电角度150/269/29，CCR3603/3063/3597，MCU A/B高侧PWM命令边沿差3.375us，power6与2.3us死区不变；最多100ms，归零/故障可提前停，正油门保持不重复。它不是持续起转版；可能小动/吸住或不明显运动。1ms后前台被动记录最多256个六路PWM数字引脚采样，停止冻结，ADC遥测更新保留。

用户短给一次油门，归零后保持供电读取即可，不需手固定转子。当前ELF改用test-firmware/fd6288-SINE-FIXED-PROBE-20261108/AM32_CUSTOM_FD6288_G431_2.20.elf；读取器在独立分支tools/scripts/fd6288_native_sine_capture.cjs，包含3096B引脚记录两读一致/边界剔除/预期电平对照。未覆盖应不同的窗口时不能下结论。构建verification.json的未刷状态是历史构建时状态，实际烧录见本目录flash.json。
