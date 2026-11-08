# SINE-SENSORS：修复正弦阶段传感器数据冻结，未烧录

基于板上SINE-CENTERED。本版仍仅正弦、power6、既有进角/速度/PWM公式和3秒截止；不改参数页、死区、幅值或六步切换策略。新增fdNativeSineSensorTick，在20kHz控制回调中沿用原one_khz_loop_counter>20门限（21ticks约1.05ms），只请求已有前台ADC处理。

旧PROCESS_ADC_FLAG=1仅在!stepper_sine分支。SINE-ONLY全程stepper_sine=1，因此电压/电流/温度更新可能一直被跳过；用户确认看的是飞控/电调遥测，旧电流读数不能用于证明绕组没有电流。此缺陷与不转是两个问题，修复它不保证电机能转。

不在控制回调中等待ADC，不启用六步PID/公共PWM写入。前台ADC regular转换和处理恢复后会增加正常传感器工作量；正弦角度更新仍按原路径执行。现有低压处理会再次得到更新的数据；本改动不新增正弦电流闭环或温控限幅。

实际C行为检查覆盖3秒内持续产生请求、请求保持、六步调度不被重置及PWM不被传感器tick改写。原逐角度PWM/仅正弦边界/ADC交接回归通过，编译通过、ELF/HEX/BIN一致。不是电机/电流物理仿真，未实测。

板上仍SINE-CENTERED，读取仍使用SINE-CENTERED匹配ELF。本包尚未烧录，原完整参数备份保留。
