# ALIGN-TAIL：定位末段快照 + 原首发记录

2026-11-06已编译并烧录，输入/运行/PWM读取为零后写入，写入及reset run成功。按用户既有要求跳过读回校验，未发送油门。见 [烧录记录](../../hardware-backups/ALIGN-TAIL-flash-20261106/flash.json)。

## 本次改动

在LAUNCH-RAW基础上仅开启FD6288_ALIGN_TAIL=1，复用同一12404B观察缓冲的slots0/1，把原第二定位59/59.8ms附近的两个快照移到第二定位约297ms和299.8ms（距计划发车约3ms和0.2ms），并在首次发车时保留下来。快照次数仍为两次，不增加ADC转换。

首发512条/40ms窗口及前2ms全组、后段隔组/强制保留ACCEPT的规则不变，RAM仍20240B。采样触发、定位内早晚配对、30+300ms定位、首发矢量、前侧资格、FIRST500、ADC一致性、TIM16调度与630/650/1300ms预算均沿用。PB7语义仍为首次发车至下一换相，不增加物理同步信号。

ABI5新增alignment_tail字段，每条包含实际程序读取时刻、距首发记录器启动时间、ABC、B−(A+C)/2、CNT/CCR快照及原定位ON窗口筛选标志。时间点为达到门槛后首次可用的消费回调，不保证恰好在297/299.8ms；若门槛附近无消费或提前停止，须看实际时间和缺失条目。

第二定位观察B浮空，首发观察A浮空；mid_error不是首发directed_margin，不能把两者符号连续拼成机械过零。两个快照若已明显不同，支持差异在切换前出现；若相近，只排除所记录两个时刻的大差异，不能证明末段全程或机械位置/速度相同。

额外保留alignment_hook_max_us：首次发车前所选配对/快照钩子的最大cycles，使用slot0.reserved存储、65535cycles封顶；首发后的hook_max_us沿用原规则。均不含完整ISR、资格检查、快照资格判定或首发头部操作，不代表无扰动。

## 验证与产物

- 仅构建CUSTOM_FD6288_G431，开/关两份；关闭ALIGN_TAIL时HEX与已烧录LAUNCH-RAW逐字节一致，SHA256 a6b4f6ccbc270f8ae7d871d6422b6514524996804e0d0d647fa80434a192222a。
- 实际pair适配器与记录器ASan/UBSan测试通过：297/299.8ms边界、一次快照、跨首发保留、重试不覆盖、窗口与条数上限、提前停止、回绕、新trial、缺失快照和ABI5解码；旧ABI2/3/4测试通过。
- RAM20240B，未增加；FLASH代码40408B。BIN/HEX一致、主程序范围检查通过，不包含bootloader和设置页；原链接警告保留。
- [完整参数](build-args.json)、[构建日志](../../docs/align-tail-20261106/)、[HEX](AM32_CUSTOM_FD6288_G431_2.20.hex)、[ELF](AM32_CUSTOM_FD6288_G431_2.20.elf)、[BIN](AM32_CUSTOM_FD6288_G431_2.20.bin)、[SHA256](SHA256.json)。HEX SHA256 7a7f68967821b28354f7ecb690b7aa45a421862c73e30eecbc7c3f200a8f92a2。
- source-base.txt/source-changes.patch保留源码基点和差异。此前LAUNCH-RAW成功/失败采集及各版固件均留存。本次未提交Git。

## 下一次观察与读取

先取得一次顺滑成功或自然失败，归零保持通电并读取；不要继续启动覆盖最新trial。读取用本目录ELF和新输出文件名，沿用原只读脚本、零输入/输出门禁、代码身份检查和冻结缓冲复读。解码器自动识别ABI5。

对照同版成功/失败：先比较alignment_tail两组ABC及标志，再比较首发首个有效样本与后续穿越。当前没有本版实测记录，不以旧ABI4相邻样本替代新快照。

首轮已读取 [trial9失败](../../hardware-backups/ALIGN-TAIL-flash-20261106/RESULT-last-failure.md)：前3ms快照通过窗口，前0.2ms未通过；首发约0.911ms接受但下一扇区未接上。下一份取同版顺滑成功。

已读取 [顺滑成功与失败对照](../../hardware-backups/ALIGN-TAIL-flash-20261106/RESULT-smooth-comparison.md)：成功首发7.936ms接受、397ms接管；定位快照均正偏但窗口筛选交错，尚不能证明定位机械状态相同。提出隔离首发矢量实验，尚未实现/烧录。
