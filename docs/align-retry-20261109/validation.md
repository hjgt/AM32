# 离线验证（未硬件实测）

- node tests/run_fd6288_start_retry_tests.cjs：实际策略/主程序适配器ASan+UBSan通过；失败与动作独立计数、所有启动反馈位置失败、3轮/截止/保护/归零及两方向回绕；实际C168B记录到JS解码通过。
- node tests/run_fd6288_align_pair_tests.cjs：原定位早晚采样/预装载/解码回归通过。
- node tests/run_fd6288_rekick_tests.cjs、node tests/run_fd6288_blue_start_tests.cjs：未启用新模式的旧适配器回归通过。
- node tests/run_fd6288_async_tests.cjs：实际异步调度截止、取消、故障、归零、方向/coast回归通过。
- build-off/on均make -B，关闭开关HEX与ALIGN-LAUNCH300归档完全相同。开启版ELF/HEX/BIN逐数据字节相符（HEX空洞按objcopy默认0填充），主程序/原设置恢复页范围及SHA核对通过。读取器使用实际保留的fdRevTick/zcfoundroutine/fdBenchService/fdBenchStop/main/loadEEpromSettings符号，不要求优化掉的静态函数名。
- 总RAM20240→20416B，新增摘要168B及策略字段；FLASH代码40480B。
- flash.py只做语法检查，没有执行；没有发油门/写目标芯片。
