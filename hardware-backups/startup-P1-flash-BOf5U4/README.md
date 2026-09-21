# 2026-09-21 08:55 P1 台架版刷入记录

用户明确授权烧录并确认停转。只写 P1 主固件，未发送油门或改设置。

旧 OpenOCD 会话内存读取失败，终止该旧会话后以原只读连接配置重新连接。
重连后第一次固件比较未通过；随后识别回读、preflight、紧邻写入前的
三份完整回读一致，才执行擦写。异常回读没有被用作烧录依据。
写前主程序确认为 A，完整 64 KiB SHA-256：
`5285553bc0420fc5c7d1689a2c96d53806fb2d4d615c16142b46a0cdb41dc5eb`。

再次确认 input/newinput/adjusted_input/running/Coast/duty=0、CCR1..4=0、
PA8..12/PF0 低后，按已验证的 `reset init` 流程停在看门狗启动之前。
只写 `AM32_CUSTOM_FD6288_G431_P1_BENCH_300ms.hex`，随后 verify_image
核验 33980 个 HEX 数据字节通过；完整回读对比确认 BL 和 F800 设置均未改。
再执行 `reset run`，停止态复核通过。

P1 HEX SHA-256：
`74619c4e6f7e149e068973c8c4f32f167e8f51fe4052cdb32c5b1224eefa66d2`。
刷后完整 64 KiB SHA-256：
`d0927d54a4d79821a5c1d55b821a3129abe57a22481e7839ce111bbb894c3fcf`。
设置页 SHA-256（未变）：
`852293fdb1c158518b9b9b4575dad3fbf74b2e7205fee5f73b44fb461b725e08`。

重启后实读：VTOR=0x08001000、主频 160 MHz、ARR=6665、DTG=0xCE；
六项运行/输入值和四个 PWM 比较寄存器均为 0，六路门极输入读数低。
armed=1，startup_min=360/minimum=210/startup_max=610；P1 额外 +40
尚未生效，因为没有起转。方向=0、堵转保护 Off、自动进角=1 不变。
电压约 8.26 V，过滤电流约 0.03 A；RAM 设置与 F800 完全一致。

trace variant=3、ARMED、样本/事件数均 0；bench ABI=1、extra=40、
trials/completed/stops/inhibit=0；策略已收到明确零油门，ready=1。
DWT 计数实读递增。未起转，不代表自动关断或起转成功率已实机验证。
下一步先由用户确认启动音/回传，再进行单次卸载的窗口关断检查。

完整时间与输出见本目录各 `*-operation-*.json`。二进制备份保留本地：
`flash64k-discovery.bin`、`flash64k-preflight.bin`、`flash64k-before.bin`、
`flash64k-after-halted.bin`、`ram32k-preflight.bin`、`ram32k-after-running.bin`。
