# S4L-ALIGN：双矢量定位＋关闭波形记录

2026-10-22，实验候选，已单target编译和主机测试，**已烧录，首轮13次仅6次进入闭环，尚不可靠**。
实测见 [首轮摘要](../../hardware-backups/startup-S4L-read-Q9MVVH/README.md)。最后一次最长连续4个边沿但300ms截止；另4次失败无有效边沿，不能以此取消无反馈总截止。
用户不希望单独测试S4J回滚，因此本候选同时引入定位并去除波形记录，不是单变量A/B实验。

主固件：`AM32_CUSTOM_FD6288_G431_S4L_ALIGN_QUIET.hex`

SHA256：`8ed6f50f7a382d79e53de25f950cf0904f29baa70d3a9cd9ecd5257fe1fe9593`

HEX数据35784字节，全部位于0x08001000..0x0800F7FF；不含BL或F800设置。
RAM4880字节（此前26448字节），FLASH代码段35280字节。
ELF：`obj-startup-S4J-align/AM32_CUSTOM_FD6288_G431_2.20.elf`。

## 控制变化

- 每次冷启动先定位两段，各30ms，不忙等。正向物理step5→6，反向step3→2，均再进入原物理step1开始反馈获取；相邻定位矢量相隔60°电角度。
- 每段占空比请求从210内部单位，在10ms内线性升至360，之后保持至本段结束。以2000为满量程，名义10.5%→18%，不是油门百分比、不是恒流控制。保留既有占空比变化率、安全上限和硬件最小ON时间约束；定位时新增硬上限防止旧占空比或stall boost绕过该包络。
- 定位时JEOS只清标志，不接受BEMF；转到获取相时，沿用commutate的检测状态复位，不能把定位产生的穿越当成接管证据。
- 两次定位换向是计划动作，不增加丢反馈次数，不伪造过零，也不累加六边沿接管置信度。
- 60ms定位占用原有300ms总获取预算，**不延长总通电获取时间**。之后首获取相仍最多30ms，沿用真实边沿、首次500µs等待、后续相位延时、救援和六边沿接管。获取阶段沿用400内部单位请求；由于总年龄已超过30ms，不再执行旧版开头的360限幅阶段。
- 定位结束不是“测得已定位”：30ms是初始试验时长，并非适用于所有电机。可能有短暂转动、反向位移或两次轻微定位动作，不保证完全无抖动。定位保持时没有速度带来的反电动势限流，必须观察实际电流和温升。
- 回零、失去运行条件、安全禁止和总截止仍使用原有停止路径；没有新增无限重试、自动重起或地面站进入超时。
- JEOS＋非阻塞定时换向、进角算法、堵转保护判定、BL和设置均未改。定位期不发生原启动超时计数，属于新增状态语义；不宣称堵转保护已实测可用。

## 记录器关闭的范围

QUIET_LOG通过编译时空操作移除全部fdTraceSample/Event/Trial等波形记录调用，移除21568字节波形缓冲区，保留fdTraceInit的DWT启用（控制算法依赖它）。
仍保留552字节启动摘要、既有ADC诊断和异步时序计数，用于确认成功/停止原因，不再存储ADC波形。
ELF检查只有fdTraceInit，无fd_startup_wave或其余fdTrace函数。
旧wave_capture工具不适用于此镜像；以后读取须使用本ELF及小型摘要，不能拿S4K ELF读地址。

## 构建与验证

`node tools/scripts/build_fd6288_s4j.cjs align` 只编译CUSTOM_FD6288_G431，启用DOUBLE_ALIGN/QUIET_LOG，关闭DENSE_LOG。

已通过：

- tests/fd6288_double_align_tests.c，ASan/UBSan：双向矢量顺序、30ms边界、每段功率请求、忽略定位边沿、DWT回绕、300ms截止、真实六边沿接管、新试验重置。
- `node tests/run_fd6288_async_tests.cjs --align` 与无开关版本：实际调度器回调、回零/故障/迟到/epoch保护及定位期JEOS禁止消费。
- `node tests/check_fd6288_s4_flags.cjs`：variant28及不兼容组合拒绝。
- `node tests/check_fd6288_image.cjs obj-startup-S4J-align/AM32_CUSTOM_FD6288_G431_2.20.bin obj-startup-S4J-align/AM32_CUSTOM_FD6288_G431_2.20.hex`。
- ARM符号检查：波形缓冲/记录函数确实不存在。
- 关闭新开关重建jeos，HEX与保存的S4J逐字节一致，证明原构建路径未被该候选改变。

仍有既有newlib未实现系统调用和RWX链接警告。主机测试不代表电气安全、定位成功或飞行验收。

## 依据与实测安排

参考 [TI Sensorless Startup Methods，第4页](https://www.ti.com/content/dam/videos/external-videos/en-us/7/3816841626001/6173360561001.mp4/subassets/tipl_sensorless_startup_methods.pdf) 的双矢量定位思路。
[ST UM3259，第6–7页](https://www.st.com/resource/en/user_manual/um3259-stm32-motor-control-sdk--6step-firmware-sensorless-parameter-optimization-stmicroelectronics.pdf) 提供定位后加速/接管的框架；本实现不是完整照搬ST开环加速算法，也未照搬其200ms默认定位时间。

已在用户确认停转后烧入，只写主固件并校验一次，未写BL/F800。日志：`hardware-backups/startup-S4L-flash-KjDMSH/flash.json`。复位运行后armed=1、油门/运行/PWM输出为0，电压8.14V，advance=23、auto=0、PWM=24、startup=150。
首轮只验证启动，固定当前12.1875°/24kHz及同一启动输入方式，不同时进行大油门测试或更改堵转开关。
先做一次观察定位动作及电流；若明显剧烈抖动或电流异常即回零，不用大油门硬拉。第一轮正常后再做小批重复启动并记录失败序号。不能因单次起转就直接认定可飞。
