# 启动后侧电平资格候选，未烧录

FD6288_STARTUP_LEVEL默认0。启用仅在fd_rev_acquiring时采用连续7个有效、消隐后、directed>8的ADC新组，无需先观察pre；反侧/滞环清计数，现有invalid分支清计数，每次换相重置，每扇区最多发布一次。闭环沿用原pre→post资格。非启动及coast不走此新资格分支。

这是公版电平累计思想的ADC实验实现，不是公版完整复刻。公版轮询约51us且有bad_count容忍；本版新组约41.67us，连续7组跨度约250us，任一非后侧有效组打断，invalid也打断。软件电平事件不是已证明的机械过零。既有confidence/edges计数和handoff在本模式下必须解释成合格电平事件，不能据六个事件直接证明机械锁定。

相对于当前ALIGN-PAIR，仅新增资格开关。保留采样实验、定位、发车角度、500us首事件等待、后续时序、TIM16及coherent ADC链路。保留500us是为了隔离改动，不称其为公版行为。没有扩展记录器或重试预算。

单目标构建参数及日志docs/startup-level-20261031；产物obj-startup-level-20261031/AM32_CUSTOM_FD6288_G431_2.20.hex。既有链接警告见build.log。未访问硬件、未烧录、未修改EEPROM。

验证：tests/check_fd6288_startup_level.py提取实际资格函数编译，覆盖双极性、无pre七组接受、六组不接受、滞环和反侧打断、一次发布、闭环仍需pre。原tests/run_fd6288_host_tests.cjs通过。主机验证不证明机械启动、ADC有效性筛选充分或ISR实际开销。

下一次实测优先一次正常起转后归零读取，关注是否直接接出首轮连续事件，以及是否立即乱跳/仍咔咔再起。不能用离线旧样本重放推算换相后的机械反应。只有用户明确归零并授权才烧录。
