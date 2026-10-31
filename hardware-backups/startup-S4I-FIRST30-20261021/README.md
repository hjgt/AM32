# FIRST30首轮：7次，另报告>50%油门停转

用户确认回零停稳；未复位/刷写，仅只读采集。
`first-trials-read.bin`及解码JSON，SHA256
`7f1e63987cbc630ff05d30034230270b2c99ddd4e1d554be51813559da170096`。
用户称高油门停止无重新上电音、回零后能再次起转；暂无与trial编号的
明确对应。读取trials7/completed7，摘要1..7连续，与本轮次数一致。

## 启动

#1/2首次边沿12.679/19.329ms，闭环回零；#3/4首次28.208/26.946ms，
确实落在新增等待内，后来进入闭环，但分别累计1/12次超时，不能宣称
新增等待直接可靠接管或这些样本在旧版必失败。#5/6首次80.537/173.236ms，
均最后闭环回零。#4最终not_running，其他#1..6为operator_zero。

#7首次30.039ms超时，前缀step1裕量+203走到-96，主要unarmed后转pre；
step2也由正侧走向负侧。不是上一版始终负侧的同一波形，不能据此
认定只是等待还不够；电压相对指令的反向跨越不证明机械反转。
唯一接受在178.097ms step5，下一相step6仍正侧unarmed；
accepted1/max_streak1/closed_zc0，15次超时，300.004ms未锁定截止。
最低7.78V、滤波电流峰值1.15A、内部duty最大400。
51条事件完整。sample最大605cycles约3.78us，非完整ISR耗时。

## 高油门停止：证据与未决点

#4摘要闭环最终not_running，冻结观察长度1276.836ms，最大duty781、
最低7.76V、滤波峰值1.45A。这些是冻结观察统计，不是停止瞬间或全程
峰值，不能用于排除瞬态压降/过流。没有对应的失败细节槽：
not_running不是当前记录器的post_handoff_loss分类，且窗口只看起步。

追加停止态只读标量：
desync_happened=1；drive_safety_limit_trip_count=0；stuck_rotor_trip_count=0；
LOW_VOLTAGE_CUTOFF=0；low_voltage_count=0；armed=1；
battery_voltage=793；actual_current=14；heli_coast_restart_inhibit=0。

Src/main.c失步分支在平均换相间隔变化超过当前间隔一半且间隔<2000
等条件下增加desync_happened，并在单向输入>47等条件置running=0；
台架fdBenchService随后可归类NOT_RUNNING并锁停至回零。
因此失步判定是与现象相容的优先调查路径，不是已证明的因果时间线。
累计计数未逐trial关联，不能直接断言#4一定就是用户>50%那次。
没有证据支持整板掉电/本轮MCU复位；也不能凭静态电压排除供电瞬态。
不关闭失步检测或继续带载升油门复现；后续需抓运行期首次异常前后
的周期、输入、PWM、状态及停止来源，区分真失步与判定误触发。
本轮不改控制代码，板上仍variant24。
