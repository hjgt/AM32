# S4L-C：早期扇区统计，控制策略保持S4L-B

2026-10-23，variant30，已编译/主机测试，已授权烧录并校验，启动统计待实测。
记录：`hardware-backups/startup-S4LC-flash-t68xzs/flash.json`。首次连接读失败发生在写入前，重启主机OpenOCD后成功。仅写主区并校验一次，BL/F800未写；复位运行后armed=1、油门/运行/PWM为0，电压7.83V，advance=23、auto=0、PWM=24、startup=150。

主HEX：`AM32_CUSTOM_FD6288_G431_S4LC_SECTOR_STATS.hex`
SHA256：`8f11ec6b44eeeab31248b74578f70f05a2fb95c69503fcd9cd11bdeedec72089`
ELF：`obj-startup-S4J-alignstats/AM32_CUSTOM_FD6288_G431_2.20.elf`
HEX36160数据字节，仅主区0x08001000..0x0800F7FF，不含BL/F800。
RAM6352字节（新增1472），FLASH代码段35656字节。

## 代码复查结论

fdDoubleAlignSeed返回4；commutate按forward增/减，所以正向物理5→6→1，反向3→2→1。
两个定位矢量与获取矢量之间都是既有相序的一步；在此逻辑层未发现跳错相。
每次changeCompInput重新设置浮空相、预期方向并清除前侧/确认/报告状态及跨边界JEOS。
定位时JEOS仅清标志，不消费BEMF；离开定位后再次commutate复位检测再开始获取。
这只是软件顺序审查，不能证明实际磁极位置、定位充分或机械运动方向。
定位结束后每步60°电角度推进的力矩效果仍依赖转子是否跟上，尚未用测量验证。

## 新增统计而非恢复波形

最新一次试验的前24个指令扇区，每扇区60字节，头32字节；不循环覆盖前缀。
记录扇区开始/结束软件时间、物理step、进入时stage和预期方向、离开原因、各判决原因计数、
样本数、有效且非blank样本的定向裕量最小/最大/首/末值、是否见前侧、最大确认数、接受边沿首/末时间。
无有效样本时极值解码为null，不把初始化哨兵当测量。
定位扇区无ADC样本是设计行为，不应解读为ADC坏了。跨界已丢弃或硬件未消费的转换不能恢复。
采样排序无法由min/max重建；这些数不能证明真实机械反转。

接管时立即冻结，停止时冻结，下一次启动覆盖。超过24扇区仅报告省略数量。
一次新扇区只清60字节，采样热路径内联，无新中断屏蔽、实时串口、逐样本时间戳或原始ADC数组。
但它仍每个已消费样本更新计数，不能宣称零开销或无时序影响；要用既有irq_max_cycles检验。
未恢复21568字节波形记录器。

## 控制与安全边界

保持两段各30ms定位、每段10ms缓升、功率请求、反馈资格、首次500µs等待、救援、360ms硬截止、回零和所有保护路径。
没有取消截止，没有新开环加速算法，没有参数/BL修改。默认SECTOR_DIAG=0。
关闭新开关重建align360，HEX与保存的S4L-B逐字节一致。

## 验证与读取

`node tools/scripts/build_fd6288_s4j.cjs alignstats` 仅编译CUSTOM_FD6288_G431。
通过`node tests/run_fd6288_sector_tests.cjs`（实际C结构/函数ASan/UBSan、边界/回绕/冻结/容量/新试验，以及C到JS ABI与损坏拒绝）、
flags variant30依赖检查、实际JEOS/异步调度回归、主区镜像及BIN一致性检查。
ARM符号确认只有fdTraceInit、无fd_startup_wave，新增fd_sector_diag为1472字节。
仍有既有newlib/RWX链接警告，没有实机时序/可靠性证明。

用户授权烧录后：先一次成功启动，回零停稳保持供电立即读取；再获得一次失败后同样读取。
不要再起转覆盖最后一次，不需要先做十几次。

```sh
node tools/scripts/fd6288_sector_capture.cjs dump obj-startup-S4J-alignstats/AM32_CUSTOM_FD6288_G431_2.20.elf NEW.bin --stopped
```

新工具仅在用户确认停转后使用，检查门控/GPIO/CCR、已知会话选定代码字节和摘要稳定性。
无reset/halt/flash/内存命令写入。只存统计、摘要及设置/异步计数；不是全Flash身份校验。
读取工具已做离线解码测试，真实硬件读取尚待本版烧录后验证。
