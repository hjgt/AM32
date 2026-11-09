# Bluejay连续启动失败上限核对（2026-11-09）

用户要求修改前先核对Bluejay，本轮不改控制代码、不编译、不烧录。

核对上游main在线源码及此前缓存commit0368d114b72f3939ea49c671b199bf0d13d9079d，两者一致：exit_run_mode_on_timeout在Flag_Motor_Running尚未置位时inc Startup_Stall_Cnt；退出关桥后clr C / subb A,#3 / jnc exit_run_mode_is_stall，因此**计数达到3便停止，连续失败最多3轮，包含首次，最多额外重试2次**。旁边注释写“>3”，以实际指令的>=3为准。计数1、2时wait100ms后跳motor_start，重新初始化；第3次失败后报警并返回arming_begin等重新解锁。normal_run_checks进入正常运行时清零计数；正常归零停止及重新arming也清零。

来源：[当前上游源码](https://raw.githubusercontent.com/bird-sanctuary/bluejay/main/src/Bluejay.asm)；[缓存固定版本对应段](https://github.com/bird-sanctuary/bluejay/blob/0368d114b72f3939ea49c671b199bf0d13d9079d/src/Bluejay.asm#L994-L1047)。本地缓存/private/tmp/bluejay-startup-review-20261105，只读核对。

我们当前ALIGN-RETRY也是总共3轮含首次（最多额外2次），关断100ms重来，连续启动失败次数上限与该Bluejay逻辑一致。正常运行后丢反馈策略未随Bluejay扩展，仍直接停机。

用户新增机械观察：100次最终成功中粗估7～8次由重试救起，最多额外重试2次；据此首轮直接成功约92～93%，是肉眼估计，不是RAM逐轮统计。后续候选要求第二定位300→100ms（第一定位仍30ms）、最多额外重试5次（含首次共6轮）。用户随后要求先查Bluejay再改；这些控制修改尚未实施，当前100次成功版本和镜像保留。
