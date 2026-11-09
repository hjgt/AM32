# 验证（已刷入，未电机实测）

node tests/run_fd6288_start_retry_tests.cjs --long-five通过真实C适配器ASan/UBSan及C288B到JS摘要解码，覆盖6轮/每轮630ms/总4280ms、30+300ms定位、100ms释放、保护/归零/方向/回绕。构建make -B成功，ELF/HEX/BIN、主程序范围和读取符号检查通过。flash.py写主程序及reset run成功，参数保留，跳过固件读回校验；只检查输入/输出停态、参数及DWT推进，无油门命令。按用户指令没有读100ms失败记录。
