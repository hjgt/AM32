# PINSCAN：复用9月ADC定位工作的低速引脚筛查候选

未烧录、未实机验证。用户飞机不在手边，本次全部离线开发。
不是飞行版，也不是高速同步过零波形记录器。

- 主固件：AM32_CUSTOM_FD6288_G431_PINSCAN.hex；配套同名ELF供读取。
- HEX SHA256：cf616bd23bb1ca1d7791bb092eeb182e4cb11f88052ef4d1cbb06b153dd831b9。
- 仅0x08001000..0x0800F7FF，不覆盖BL/F800参数。RAM16232字节。
- variant32；沿用S4L-B/C控制时序，不引入S4M新的早期换相策略。
- 扫描PA3、PA4、PA7、PB0，保留PA5电压、PA6电流对照；SWD不动。
- ADC1 regular温度rank保留，ADC2原传感器rank1/2保留；不改三相注入配置。
- 约1kHz后台采样，每窗口512组，约0.5秒。上电记录静态，起转后重新记录；停机/满窗冻结，下次起转覆盖。
- GPIO只做掩码后的读取，不改变PA15/PB3/4/7/PF1模式；模拟模式数字电平无效。
- 无法从本版的异步慢采样确认中性点/过零边沿；先筛选，再决定针对候选做同步高速记录。

构建：`FD6288_PIN_SCAN=1 node tools/scripts/build_fd6288_s4j.cjs alignclean`。
关闭：`node tools/scripts/build_fd6288_s4j.cjs alignclean`；已验证关闭后HEX与原clean基线逐字节一致。

停稳后读取：
`node tools/scripts/fd6288_pin_scan_capture.cjs dump test-firmware/fd6288-PINSCAN-20261023/AM32_CUSTOM_FD6288_G431_PINSCAN.elf NEW.bin --stopped`

工具要求现成的只读OpenOCD服务；不halt/reset、不写RAM。冻结及双读一致后保存BIN/JSON。
上板时先检查静态电压/电流遥测未异常，再由用户决定短时起转，回零后读取。
不会同时修改进角、PWM或关闭停机保护。

已通过：单目标编译、记录器边界/启动/冻结/重启/ABI主机测试、解析器边界及通道顺序、
开关组合、HEX/BIN一致性和主固件地址边界检查。保留既有newlib syscall/RWX链接警告。
硬件运行时regular/injected仲裁和记录干扰尚未测量，不能保证零影响。

历史来源与完整方案：[复查记录](../../docs/FD6288-PIN-RESCAN-20261023.md)。
