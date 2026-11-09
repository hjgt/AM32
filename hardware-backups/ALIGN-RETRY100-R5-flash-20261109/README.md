# ALIGN-RETRY100-R5已烧录（2026-11-09）

用户明确通知烧录。当前旧300ms ALIGN-RETRY匹配ELF归零/输出关闭门禁通过，备份当前完整2048B参数页并确认仍与原参数一致。仅新主程序写入41288B和reset run成功；参数页保留，按用户要求跳过固件读回校验，未发送油门。

启动后再次确认输入/输出全关，RAM参数前48B与原参数相同(sine0/PWM24K/startup150%/KV900/14极/advance18、auto0)，DWT计数使能并推进，见attempt1/flash.json。刷入前参数另存attempt1/settings-before.bin。

当前板上30+100ms双定位，正常功率与反馈换向时序保留；启动缺反馈关断100ms，从第一定位重来，最多额外5次、含首次总6轮；单轮430ms/启动总3080ms软件截止，耗尽后归零重来，运行丢反馈仍停止。成功后完整持续运行，没有100ms停机上限。

尚无本版电机实测，旧300ms版100次最终成功不算本版结果。用户可测试后归零停转保持供电，读取tools/scripts/fd6288_start_retry_capture.cjs＋test-firmware/fd6288-ALIGN-RETRY100-R5-20261109/AM32_CUSTOM_FD6288_G431_2.20.elf；摘要ABI2/288B，不能用旧包ELF地址读当前RAM。

新HEX SHA256 d6978ad73ddfcb6e9c1ddcfe360ed974dc08547337c7b33a674e09a97185ccca。旧300ms/3轮成功版包、参数及专用源码分支保留。
