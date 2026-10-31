# PINSYNC（variant33）：PA3/PA7同源PWM触发采集

已离线编译/检查，未烧录、未实机验证。不是飞行版或新的换相控制算法。

主HEX：AM32_CUSTOM_FD6288_G431_PINSYNC.hex；配套同名ELF用于读取。
SHA256：04f893e3c8bce6f0be7982b5ec5882f3ada8c6b1509262dfa2ca89b31275f2e9。
主固件0x08001000..0x0800F7FF，BL/F800不在镜像范围。RAM16224B。

基于alignclean/S4L-B/C策略，启动时序、停止保护、进角、PWM和EEPROM不改。
ADC1 injected追加第四rank PA3；ADC2 injected单rank PA7，两者同TIM1_CH4下降事件触发。
原三相rank1..3及温度/电压/电流regular配置保留，但追加rank使JEOS理论延后0.475µs，
独立ADC仲裁及记录工作也有开销，不能宣称完全不影响控制。

闭环DRIVE后等200ms，再记录512组（24kHz下约21.3ms）并冻结，下一次起转覆盖。
每组五路raw和PWM/扇区信息；时间戳是软件读取时刻，不是硬件采样时间戳。
ADC2新鲜度、完整ON窗口、连续周期、换相blank和三相有效性共同筛选；
窗口不足不提高输出，仅标记记录无效。不能从一次记录直接确认中性点连接。

构建：`FD6288_PIN_SYNC=1 node tools/scripts/build_fd6288_s4j.cjs alignclean`。
关闭：`node tools/scripts/build_fd6288_s4j.cjs alignclean`，已确认与原clean基线HEX逐字节一致。
PIN_SCAN与PIN_SYNC互斥；不恢复旧的大型启动记录器。仅编译CUSTOM_FD6288_G431。

烧录后按已验证油门正常起转约1秒，回零停稳再读：
`node tools/scripts/fd6288_pin_sync_capture.cjs dump test-firmware/fd6288-PINSYNC-20261023/AM32_CUSTOM_FD6288_G431_PINSYNC.elf NEW.bin --stopped`

不正常立即回零，不需要继续加油补齐记录。若未进入闭环或过早停止，可能没有完整窗口。
读取只读、要求冻结及完整双读一致，输出BIN/JSON和符合筛选条件的样本统计。

通过：状态机/时钟回绕/门控/冻结/重启/结构大小、解析器、开关组合、
调度器/JEOS门控、旧ADC/换相/故障主机回归、HEX/BIN一致与主固件范围检查。
保留原有newlib syscall/RWX链接警告。未测IRQ最大耗时、输入RC建立误差和ADC仲裁。

完整设计与局限见[同步采集说明](../../docs/FD6288-PIN-SYNC-20261023.md)。
