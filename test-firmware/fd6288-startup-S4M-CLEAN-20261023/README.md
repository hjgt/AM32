# S4M：原生风格早期换相＋关闭观察诊断（2026-10-23）

候选，2026-10-24已烧录并完成停机态检查，起转实测待用户反馈。不是可飞行最终版，也不代表堵转保护已验证。

烧录日志：hardware-backups/S4M-flash-TbU01w/flash.json。只写主HEX，verify33836字节通过；
复位后armed=1，油门/running/duty/CCR全0，ADC1恢复3rank，ADC2 JSQR=0。

主候选 `AM32_CUSTOM_FD6288_G431_S4M_NATIVE_CLEAN.hex`，variant31。
SHA256 `f9811ed08011a1985bba9c3d15ff7f082113debeb96e9ac5ffcb970be843b2f2`。
配套ELF `S4M_NATIVE_CLEAN.elf`。只写主固件0x08001000..0x0800F7FF；不写BL或F800设置。

## 改动和依据

- 保留双矢量定位各30ms、每段10ms缓升210→360、原无反馈重试节奏和360ms总截止。
- 前五个连续合格过零后立即请求换相，不再故意等首次500µs、后续最多3ms。
  实际仍有ADC量化、软件执行和驱动延迟，不是物理零延迟。
- 第六个边沿的实测周期接管、闭环JEOS/定时换相及进角设置不变。
- 参考[AM32 v2.21 main.c](https://github.com/am32-firmware/AM32/blob/v2.21/Src/main.c)中
  zcfoundroutine在zero_crosses<5时退出等待的路径；用户同负载使用AIKON_SINGLE_F421 v2.21正常。
  F421比较器前端与本板三相ADC不同，不声称等效完整原生算法，也未复制原生强制超时计为过零的逻辑。
- 本轮失败样本trial3在首获取相719个有效样本全部为过零前极性、最终零合格边沿。
  它没有走到上述延时，因此这个时序改动不能单独解释或保证修复该失败。
  成功trial1六边沿接管约122.272ms；失败trial3截止360.009ms。
  原始证据：hardware-backups/startup-S4LC-flash-t68xzs。

## 一键移除诊断

`node tools/scripts/build_fd6288_s4j.cjs alignnative`

只编译CUSTOM_FD6288_G431。NO_DIAGNOSTICS=1编译移除ADC记录环、扇区/波形/事件记录、
启动摘要、计数和IRQ耗时统计写入。保留DWT时基、控制状态、有效边沿计数和保护门控。
RAM3904字节（S4L-C为6352）；已检查ELF无adc_zcd_diag/trace、fd_power_work、fd_startup_wave、fd_sector_diag。
fd_async_diag名字保留，但其中deadline/pending/epoch等是控制状态，不能删除；诊断计数不再更新。

恢复轻量诊断：`FD6288_DIAGNOSTICS=1 node tools/scripts/build_fd6288_s4j.cjs alignnative`。
使用独立-diag对象目录，波形仍由QUIET_LOG禁用。扇区统计另用alignstats模式。
关闭诊断后不能再用旧捕获工具/旧ELF来判断成功次数或失败原因。

保留360ms未接管截止、闭环丢反馈停止、换相迟到超过50µs停止、回零/安全/原生堵转门控。
这些虽源于试验代码，已经影响输出安全，不属于本次移除的“纯观察代码”。
闭环1秒标志用于结束启动支持、防DWT回绕，不是1秒停机。

## ADC频率核算

没有改EEPROM PWM或ADC时钟。沿用当前固定24kHz设置时每PWM一次三相注入转换，即24k组三相/s。
最低采样窗口：硬件死区368ticks + 稳定裕量92ticks + 三相转换及裕量244ticks
=704/160MHz=4.4µs。低占空比210/2000在24kHz仅4.375µs，已触及窗口钳位。
48kHz同占空比仅2.188µs，不足；若维持4.4µs窗口，实际最低输出需约21.1%，并非单纯提高采样率。
因此本轮先减小IRQ负担，不声称提高实际采样率；高频方案需另评估高占空比多采样、
ADC时钟/输入RC稳定时间和采样点，不能盲目缩短采样时间。定位期不消费过零，照旧。

## 对照与测试

可选 `AM32_CUSTOM_FD6288_G431_S4LB_CLEAN_CONTROL.hex`：仅关闭诊断，保留之前控制时序。
SHA256 `d19dad2774a4208d326473b07a8c71f7c977d35c8c1998c074642c5d753279c7`。
构建命令 `node tools/scripts/build_fd6288_s4j.cjs alignclean`。
此文件用于需要时分离记录器/时序影响，不要求先做回滚测试。主候选先保持现有EEPROM参数不变。

已通过新旧时序×诊断开关的状态机测试、双方向/计时回绕、六真实边沿接管、截止不续期、
诊断开关两态的真实调度器/JEOS门控测试、扇区记录器及参数组合测试；主HEX/BIN一致和地址边界检查通过。
主机仿真不是电机物理模型，不能证明功率级安全或成功率。
推荐在固定起转油门下记录成功/失败次数及现象；异常立即回零，不通过持续加油强行恢复。
