# ALIGN-RETRY已烧录（2026-11-09）

用户明确通知“烧录吧加电了”。先只读确认油门/input/newinput/running/duty/coast及六路引脚/CCR输出关闭，备份当前完整2048B参数页。写入新主程序41024B、原参数页2048B及reset run成功；按用户既有要求跳过固件读回校验，未发送油门。

首次attempt1仅因旧OpenOCD会话read_memory失败退出，未halt/reset/写入；关闭旧本地会话、100kHz普通init重新识别后，attempt2成功。ST-Link目标电压约3.259V。

启动后再次确认输出关闭，运行参数前48B与原参数备份相同，sine0、PWM24K、startup150%、KV900/14极、进角18/auto0。DWT已使能且推进，详见attempt2/flash.json最后runtime_clock。此处是运行态检查，没有做固件读回验证或电机实测。

当前板上为ALIGN-RETRY，恢复完整启动运行，不再100ms自动停止。30+300ms定位、正常500us首反馈与6反馈接管；启动缺反馈关断100ms后从第一定位重来，总共3轮含首次、单轮630ms/全程2090ms获取上限，耗尽后归零才重来，运行丢反馈仍停机。

读取必须使用test-firmware/fd6288-ALIGN-RETRY-20261109/AM32_CUSTOM_FD6288_G431_2.20.elf和tools/scripts/fd6288_start_retry_capture.cjs。测试后归零停转保持供电，读取最新168B逐轮摘要。不要继续用CLAMP或旧EARLY_REKICK ELF解析当前RAM。尚无本版电机运行结果。

固件HEX SHA256 6629c95009f48f5ab1e1608850113f449dda9b8589df1155dd1550bb020b3284；恢复原参数页SHA256 4f426d535486cb0d9b9c8b7bc0b74cd390efb56c73e9116e3a64f6c5a7baf7f3。刷入前当前正弦参数另存attempt2/settings-before.bin。源代码分支codex/fd6288-align-retry-20261109、实现10b8c10、产物716b01d。
