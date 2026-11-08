# A→B对照已完成，烧录待SWD恢复

用户同意进行B返回通路验证，已编译与检查，通过后尝试直接烧录。构建提交ae60f05；A相PWM/B相拉低/C相浮空，24K、18%给定、compare1200、原死区、100ms边界与记录均保持；仅由step6改为step1。关闭新开关HEX与当前A→C版相同。SHA256 cb6b8cb108d6236d7b6fc8fd6a0aab004a421b1546d8842d136280eb80805f42。

旧OpenOCD归零预读VTOR失败；关失效会话后常规100K/50K均识别ST-Link约3.239V，但target init失败。没有执行reset/flash，没有发送油门。已请用户接稳供电/SWDIO/SWCLK/GND，归零后继续。失败记录flash-attempt1-target-unavailable.json；恢复服务后运行flash.py。

板上仍A→C的SIXSTEP-ALIGN-CONTROL，当前读取继续匹配原A→C ELF。未成功烧录前不能用新A→B ELF读旧RAM。新解析器用summary.last_step选择step1/6预期，因为两种模式GPIO MODER相同、恒高低侧命令不同。

用户观察六步对地占空比更低是合理的：18%是六步定时器给定，功率端受死区/续流影响；正弦各相接近50%含共模，不能直接以两者数值推功率。旧六步B浮空，不是受控PWM；新A→B版B将固定接近电源负极、C浮空。
