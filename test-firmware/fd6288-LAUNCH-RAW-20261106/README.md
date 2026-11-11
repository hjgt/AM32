# LAUNCH-RAW：300ms定位后首发ADC记录

2026-11-06已编译并烧录，写入及reset run均成功，跳过读回校验。恢复SWD连接后读取到输入/运行/PWM为零才执行写入；未发送油门。见 [成功烧录记录](../../hardware-backups/LAUNCH-RAW-flash-20261106/flash.json)。

## 要回答的问题

此前300ms发车版同会话trial1..6顺滑接管，第7次失败：首发后约0.339ms接受首事件，约0.515ms后换相，下一扇区22.54ms无接受。需要看首发ABC、判决分类及首个换相前后关系；不能先认定是假过零。见 [失败分析](../../hardware-backups/ALIGN-LAUNCH300-flash-20261105/RESULT-last-failure.md)。

## 降低记录扰动的实现

- 基于已烧录ALIGN-LAUNCH300，仅开启FD6288_LAUNCH_RAW=1。保持30+300ms定位、首发矢量、前侧资格、FIRST500、ADC触发与TIM16调度；启动/重试预算仍为630/650/1300ms。
- 保留原ALIGN-PAIR定位采样时点切换。首次发车桥配置完成后，以ABI4复用原12404B观察缓冲，只清头部计数/标记，不清12KB数据，不新增RAM分配。原配对数据会被首发记录替换，已有定位bins仍在。
- 只复制getAdcComparatorLevel已经读出的ADC三相值，不新增ADC触发或转换，不插入轮询、等待或串口输出。
- 从首次发车记录器启动起，前2ms保留每组实际消费的ADC数据；其后隔组保留，但ACCEPT总会保留。每组最多写一条24B样本加计数；40ms或512条到上限即冻结，后续停止不覆盖冻结终点。正常24kHz消费时，头段约41.7us一组、后段约83.3us一条，实际缺组以软件记录为准。
- 样本含ABC、方向化margin、分类原因、step/sector、valid/blank相关标记和读出时CNT/CCR快照。时间戳是读取ADC的程序时刻，不是硬件采样时刻，更不是外部示波器波形。
- 新trial重置为ABI3配对模式；首次发车切ABI4。重试定位不重新开启配对或覆盖首发记录，对应原基线首轮配对已满后的行为。
- max_cycles/calls在首发切换时重置，统计所选记录钩子的耗时；不包括完整ISR、资格检查和首发头部重置。重试定位的既有pair钩子调用也可能更新该开销统计。需实机读出，不能仅凭代码证明时序完全不变。

## 与外部触发、数据保留的关系

PB7仍是首次发车桥配置完成后拉高、下一次换相/关断拉低，约330ms开始；没有变为定位标记。ABI4的start0是在同次commutate中稍后执行记录器初始化的程序时刻，不能与PB7当成零偏差。

仅保留最新trial的波形记录，后续启动会覆盖。取得成功或自然出现失败后归零并保持通电，不再启动，使用本目录ELF读取。无需为记录做大量重复启动；前次失败文件已经独立归档，不会随烧录丢失。

读取命令（已归零后）：

```sh
PATH=/Applications/ArmGNUToolchain/15.2.rel1/arm-none-eabi/bin:$PATH node tools/scripts/fd6288_sector_capture.cjs dump test-firmware/fd6288-LAUNCH-RAW-20261106/AM32_CUSTOM_FD6288_G431_2.20.elf hardware-backups/LAUNCH-RAW-flash-20261106/capture-NEW.bin --stopped > hardware-backups/LAUNCH-RAW-flash-20261106/read-NEW.log 2>&1
```

每次使用新文件名。仍执行只读归零门禁、匹配代码身份和冻结缓冲复读；不halt/reset。解码器自动识别ABI4。

## 离线验证与产物

- 仅构建CUSTOM_FD6288_G431，开启/关闭各一份。关闭记录开关的HEX与当前ALIGN-LAUNCH300逐字节一致，SHA256 bfe2c4a8fefae61bb0506f8167960624a477752443ac65a5d6f8339e0850035f。
- RAM20240B，未增加；开启版FLASH代码40352B。原newlib/RWX警告保留。主程序镜像范围、BIN/HEX一致性检查通过，不含bootloader/设置页。
- 实际pair适配器及新记录器在ASan/UBSan下测试通过：保留早晚配对、头部复用、前2ms全组、后段抽取、强制保留接受事件、跨扇区、512条及40ms冻结、提前停止、重试不覆盖、DWT回绕、新trial和ABI4解码。旧ABI2记录器及ABI3配对回归通过。
- 参数、日志见 [构建目录](../../docs/launch-raw-20261106/)。[HEX](AM32_CUSTOM_FD6288_G431_2.20.hex)、[ELF](AM32_CUSTOM_FD6288_G431_2.20.elf)、[BIN](AM32_CUSTOM_FD6288_G431_2.20.bin)、[哈希](SHA256.json)。HEX SHA256 a6b4f6ccbc270f8ae7d871d6422b6514524996804e0d0d647fa80434a192222a。
- source-base.txt与source-changes.patch保存Git基点和已跟踪差异。此前各版产物未覆盖，尚未提交Git。

实机待验证：启动表现相对基线是否改变、记录钩子开销及成败首发波形。首轮烧录前SWD失败记录保留为flash-attempt1.json；连接恢复后的十六进制回复解析问题发生在写入前，已修正并保留flash-attempt2.json，最终flash.json为成功写入记录。

首轮已读取：[trial13失败分析](../../hardware-backups/LAUNCH-RAW-flash-20261106/RESULT-last-failure.md)。505条记录；首发有效样本全后侧，首扇区30ms超时，局部钩子最大1.38125us。下一份取直接顺滑成功对照。

已读取 [trial14顺滑成功与trial13失败对照](../../hardware-backups/LAUNCH-RAW-flash-20261106/RESULT-smooth-comparison.md)：第一有效margin分别−82/+66，成功18.812ms接受、零超时连续接管。下一项建议补定位最后几毫秒，尚未修改固件。
