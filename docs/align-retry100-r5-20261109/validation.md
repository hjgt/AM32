# 验证记录

实际执行通过：

- node tests/run_fd6288_start_retry_tests.cjs --short-five：真实C策略/适配器ASan+UBSan，30+100ms定位、6轮所有成功位置/所有失败位置、430ms/3080ms截止、保护/归零/两方向回绕，C288B→JS。
- node tests/run_fd6288_start_retry_tests.cjs：旧300ms/3轮与168B解码回归。
- node tests/run_fd6288_align_pair_tests.cjs：既有ADC定位采样/预装载/解码回归。
- 两个make -B构建，legacy300产物与已安装旧包HEX完全相同；short100-r5镜像ELF/HEX/BIN一致、仅主程序范围、摘要288B及实际读取函数符号检查通过。
- flash.py仅语法检查，没有执行，目标芯片/参数未修改。
