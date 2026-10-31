# 2026-09-20 晚间：A 记录对照版已刷入

用户明确授权“刷吧”，保持油门 0、转子停稳、电池供电。
此次只更新主固件，不改变 BL/F800 设置，不发送油门。

## 刷写流程与核验

1. 写前再次实读 input/newinput/adjusted_input/running/Coast/duty=0，
   PA8..PA12、PF0 低电平，TIM1 CCR1..4=0。
2. 最新完整 64 KiB 回读保存为 `flash64k-before.bin`，与本次 preflight
   和之前设置修复后的镜像逐字节一致。
3. `reset init` 在主程序启动 IWDG 之前停核；保持 SWD 400 kHz，避免仅
   halt 后执行 Flash 算法被看门狗复位的已知问题。
4. 使用 `flash write_image erase` 写 A 主 HEX；`verify_image` 校验
   30848 个实际数据字节通过。没有整片擦除、BL 写入或设置页写入。
5. 停核状态完整回读 `flash64k-after-halted.bin`。主 HEX 全部记录一致，
   0x08000000..0x08000FFF（BL）和 0x0800F800..0x0800FFFF（设置）与
   写前备份逐字节相同。然后执行 `reset run` 正常重启。

完整操作时间和工具输出见 `operation.json`。

## 固件与哈希

源码候选提交 `79ee644`，A 为 `FD6288_STARTUP_TRACE=1`、
`FD6288_STARTUP_TIMING_FIX=0`，不是 B 时序修正版。
主 HEX：`obj-startup-trace-A/AM32_CUSTOM_FD6288_G431_2.20.hex`。
SHA-256：`24ff2551ca5908ce1afc421003b9a8e4d0725958540749ce3239f9449c7a82ee`。

刷后完整 64 KiB SHA-256：
`5285553bc0420fc5c7d1689a2c96d53806fb2d4d615c16142b46a0cdb41dc5eb`。
设置页 SHA-256（未变）：
`852293fdb1c158518b9b9b4575dad3fbf74b2e7205fee5f73b44fb461b725e08`。

## 重启后停止态实读

见 `runtime-after-running.json` 和 `ram32k-after-running.bin`。

- CPU running、VTOR=0x08001000，SystemCoreClock=160 MHz。
- input/newinput/adjusted_input/running/Coast/duty 全为 0；armed=1，
  已接受零油门并解锁待命，但未运行电机。
- PA8..PA12/PF0 低，TIM1 CCR1..4=0、ARR=6665。
- minimum/startup/startup-max=210/360/610，方向=0、堵转保护=0。
- RAM 的 192 字节设置与 F800 对应内容一致，Flash 设置页也与写前一致。
- 记录器 magic/ABI 正确、variant=0（A）、state=1（ARMED），capture_id=0，
  样本/事件数均为 0；DWT 时基已启用且实读递增。
- 电压约 7.71 V，过滤电流约 0.05 A。

只完成刷写和停止态核验，未测试本版鸣叫/飞控回传、地面站、起转或带载。
下一步先由用户确认基本表现，再按晚间计划采样；不得把本次写入成功
当成偶发起转问题已修复。A 保留原控制决策，先用于对照。
