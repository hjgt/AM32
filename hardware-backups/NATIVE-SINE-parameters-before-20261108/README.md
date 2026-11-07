# 原参数只读备份，2026-11-08

为原生正弦启动试验保存修改前参数。匹配当前 EARLY-REKICK 代码后读取，
完整设置页连续两读一致；本轮没有暂停、复位、写 RAM、写 Flash 或发送油门。

- `settings-page.bin`：`0x0800f800..0x0800ffff`，原始 2048 B。
- `runtime-settings.bin`：eepromBuffer 原始 192 B，与设置页前 192 B 一致。
- `restore-settings.hex`：将原始 2 KB 页原样封装，供以后恢复，不包含主程序或 Bootloader。
- `capture.json`：读取时间、匹配 ELF、只读操作范围和原值。

原值：Startup power 150、900 KV、14 极、正弦启动关闭、range 5、power 5；
进角编码 18、自动进角 0（固定 7.5°）。新包保留进角字节，只改变正弦开关和 range/power。

原设置页 SHA256：
`4f426d535486cb0d9b9c8b7bc0b74cd390efb56c73e9116e3a64f6c5a7baf7f3`。
