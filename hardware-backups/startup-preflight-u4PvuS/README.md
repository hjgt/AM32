# 2026-09-20 晚间起转测试前只读核验

用户确认油门 0、转子完全停稳、电池已供电后，使用 ST-Link V2 / SWD
400 kHz 连接。只读服务禁用 GDB/telnet 和 examine-end 寄存器修改。
本次没有 halt、reset、Flash 写入、设置保存或油门操作。

- `flash64k.bin`：0x08000000 开始的完整前 64 KiB。
- `settings-f800.bin`：该备份中的 F800..FFFF 设置页。
- `ram32k.bin`：主程序运行、转子停稳时的 RAM 快照，非原子快照。
- `inspection.json`：读取时间、读取前后寄存器、固件逐字节匹配与运行参数。

主程序实际 HEX 数据逐地址匹配 `2c1b5fa`（28588 字节），不是 A/B 记录版。
完整 Flash SHA-256：
`65b582318e42f338fbe736b63d246003383b160f499f09bb47f33efbd2b047d7`。
设置页 SHA-256：
`852293fdb1c158518b9b9b4575dad3fbf74b2e7205fee5f73b44fb461b725e08`。
两者均与之前修复后的备份相同。

读取前后 CPU running、VTOR=0x08001000、PA8..PA12/PF0 低、CCR1..4=0。
RAM input/newinput/adjusted_input/running/Coast/duty 均为 0。
minimum/startup/startup-max=210/360/610；PWM=24、auto_advance=1，
方向=0、stuck_rotor_protection=0。遥测采样约 7.74 V、0.05 A。
只验证了停止态，未验证起转。已询问是否允许刷入 A 版；写入前还须即刻
复核零输入和关桥状态，并保留 BL/F800 页。
