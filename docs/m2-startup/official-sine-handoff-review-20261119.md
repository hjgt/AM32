# 官方 AM32 正弦交接与加速对照（2026-11-19）

对照成功设置使用的官方 v2.16，并核对官方 v2.21。结论：官方正弦退出后走六步过零控制，实际输出沿用原有启动、低速、高速占空比斜坡；没有本次试验新增的固定 20% 上限和等待 24 次真实反馈阶段。M2 本次撤去的就是这层附加限制。

## 正弦到六步

[v2.16 正弦出口](https://github.com/am32-firmware/AM32/blob/v2.16/Src/main.c#L2203-L2246) 在退出条件满足时等待指定电角位置（较高输入可强制退出等待），设置 `stepper_sine=0`、`running=1`、`old_routine=1`，用 `commutation_interval=9000`、`zero_crosses=20` 作软件种子，选择 `changeover_step` 并换相。这里的 20 是赋值，不是已经测得 20 次真实过零；没有 M2 当前的全关辨相和九个合格事件锁定过程。

[v2.21 正弦出口](https://github.com/am32-firmware/AM32/blob/v2.21/Src/main.c#L2306-L2351) 继续保留上述基本流程。M2 的 500 RPM、全关辨相、锁定接管已通过前期测试，本轮不因参考官方而重做这些部分。

## 接管后的油门

[v2.16 输出斜坡](https://github.com/am32-firmware/AM32/blob/v2.16/Src/main.c#L1482-L1527) 按 `zero_crosses`、当前占空比及平均换相间隔选择启动、低速、高速最大变化量，再限制实际 `duty_cycle` 相对 `last_duty_cycle` 的变化。它不是让六步长期固定在某个转速，也不是额外固定 20% 后定时释放。[v2.21 原有斜坡](https://github.com/am32-firmware/AM32/blob/v2.21/Src/main.c#L1466-L1500) 延续同类控制。

M2 的正常油门映射和既有输出斜坡原本已存在。本轮删除 `fd_m2_throttle` 状态、20% 请求钳位、24 次反馈等待和每毫秒额外释放上限，不删除原生斜坡，也不改 EEPROM 的 max_ramp 参数。接管瞬间的 400/2000 输出种子继续使用，随后由原有逻辑接续。

## 高转速过零

[v2.16 低速轮询处理](https://github.com/am32-firmware/AM32/blob/v2.16/Src/main.c#L1619-L1665) 与[高速比较器中断和换相定时](https://github.com/am32-firmware/AM32/blob/v2.16/Src/main.c#L968-L1028) 分工：高速中断确认比较器电平，读出/重置间隔计时并安排换相定时器；回调换相并更新周期滤波及进角。

[官方 v2.21 G431 比较器](https://github.com/am32-firmware/AM32/blob/v2.21/Mcu/g431/Src/comparator.c#L35-L87) 读取硬件比较器输出并配置 EXTI。M2 的板级实现通过 24 kHz PWM 同步三相 ADC 判断过零，正常相邻组约 41.7 µs，另有 PRE/POST、滞回及换相屏蔽资格。官方比较器事件能发生在两组 ADC 之间，不能将其延迟和 15° 设置直接视作本板经过标定的实际进角。当前有界插值补偿和 DShot 实发周期记录继续保留；失败 trial 4 的补偿执行次数为零。

本轮只撤去新增油门缓升，不同时改变过零补偿门槛、接管策略或采样冻结条件，便于隔离这层限制的影响。低速异常记录缺口已在 sine-coast.md 中记录，留作后续诊断调整。
