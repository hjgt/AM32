# M2 PURE · FD6288 / STM32G431

这是 **M2 机型** 的纯净固件归档分支。固件内置文件标识为 **M2_PURE_FD6288_G431**。硬件目标仍为CUSTOM_FD6288_G431；请使用本分支的M2构建配置。

| 机型 | GitHub版本 | 行为 |
|---|---|---|
| M1 | [2026-11-20 PURE](https://github.com/hjgt/AM32/releases/tag/m1-fd6288-g431-pure-2026-11-20) | 保留11月11日控制，通过总开关关闭调试采集与统计 |
| M2 | [2026-11-20 PURE](https://github.com/hjgt/AM32/releases/tag/m2-fd6288-g431-pure-2026-11-20) | 500RPM正弦接管，三秒缓启动，高速ADC修复，无调试采集 |

M1主分支为`m1-main`，M2主分支为`m2-main`。两条发布线独立，M2不替换M1的Latest标记。全部M2发布附件以`M2_`开头。

## M2控制与pure含义

- 保留500RPM正弦、定向保持、自适应相位/速度锁定与六步接管。
- 保留已验证的三秒输出缓升：接管20%输出到满输出的名义释放时间3s；运行中突然加油门也限升，反馈不良暂停，恢复不补涨。降请求、归零和保护优先。
- 保留高速有效前侧筛选、真实样本过零时间补偿、驱动窗口/一致性检查及失步停机。
- 删除ADC波形环、DShot实发历史环、滑行/带电采样数组、事件日志、峰值/耗时/试验统计。仅保留驱动、接管、斜坡和保护所需状态。
- 正常DShot转速回传、电流/电压/温度采样及已启用保护继续工作。
- 相比已验证的M2三秒诊断版，pure仅移除观测与统计并更名。控制等价主机测试通过；pure构建仍需实机复测，因此作为Pre-release归档。

## 烧录与设置

本Release提供完整三段，各有BIN和HEX：

| 区域 | BIN地址 | 长度 | 来源 |
|---|---|---:|---|
| Bootloader | `0x08000000` | 4096B | 11月11日PB5 V17，逐字节复制 |
| M2 PURE应用 | `0x08001000` | 59392B | 从本分支最新发布提交构建 |
| M2参数页 | `0x0800f800` | 2048B | 原生Ramp试验写入的0.1%/ms完整备份，逐字节复制 |

HEX带绝对地址；BIN须指定表中地址。应用HEX/BIN本身只覆盖应用区。Bootloader和参数作为独立附件提供，源文件与哈希保存在`release/m2-pure/`。参数页`max_ramp`为raw1（界面0.1%/ms），与昨晚试验写入页完全一致；没有重新生成或移植M1参数。M2闭环仍使用三秒控制器，此字段不决定三秒上升速度，补入参数页也不意味着恢复原生Ramp算法。

固件文件标识为`M2_PURE_FD6288_G431`。每次加载仍沿用既有M2目标RAM设置，没有增加自动写参数Flash。

M1 Release的附件属于M1归档；M2下载请从上表M2链接进入。[11月11日历史M1 Release](https://github.com/hjgt/AM32/releases/tag/m1-fd6288-g431-final-2026-11-11)仍保留原tag和固件附件。

## 构建与检查

依赖Arm GNU Toolchain、make、Python3；主机测试还需Node.js与支持ASan/UBSan的C编译器。

```sh
python3 tools/scripts/build_m2_pure.py
node tests/run_fd6288_native_sine_tests.cjs --m2-pure
node tests/fd6288_m2_pure_equivalence_tests.cjs
```

构建参数完整保存在`docs/m2-startup/build-args-m2-pure.json`，普通目标默认选项不代表M2发布配置。应用由500RPM成功版恢复，所有保护阈值保持原有策略；不新增4A台架停机。

检查包括真实87%/89%失败波形经实际判定和控制器回放、旧规则漏检对照、过早/轨端/碰撞/旧组/采样点切换拒绝、反向和回绕；三秒纯净/诊断控制器100000tick输出及控制状态一致；配置电流/温度/运行状态/归零停止；ELF无调试采样数组、应用区边界及制品指纹检查。RAM约4KB，具体以发布构建清单为准。

## 历史与许可

M2分支以M1的2026-11-11发布提交为祖先，保留AM32原始历史与GPL-3.0许可。原上游说明见下文。M1标签/分支重命名对照位于`docs/m2-startup/m1-naming-20261120.json`；重命名未改旧Release固件资产。

---

# AM32-MultiRotor-ESC-firmware
Firmware for ARM based speed controllers
<p align="left">
  <a href="/LICENSE"><img src="https://img.shields.io/badge/license-GPL--3.0-brightgreen" alt="GitHub license" /></a>
</p>

The AM32 firmware is designed for STM32 ARM processors to control a brushless motor (BLDC).
The firmware is intended to be safe and fast with smooth fast startups and linear throttle. It is meant for use with multiple vehicle types and a flight controller. The firmware can also be built with support for crawlers. For crawler usage please read this wiki page [Crawler Hardware](https://github.com/AlkaMotors/AM32-MultiRotor-ESC-firmware/wiki/Crawler-Hardware-and-AM32)

## Features

AM32 has the following features:

- Firmware upgradable via betaflight passthrough, single wire serial or arduino
- Servo PWM, Dshot(300, 600) motor protocol support
- Bi-directional Dshot
- KISS standard ESC telemetry
- Variable PWM frequency
- Sinusoidal startup mode, which is designed to get larger motors up to speed
## Build instructions
Download and install Keil community edition. Open the Keil project for the mcu you want in the "Keil projects" folder. Install any mcu packs if prompted. 
Select the build target from the drop down box and build project 

## Firmware Release & Configuration Tool

The latest release of the firmware can be found [here](https://am32.ca/downloads).

The primary configurator is the [AM32 Configurator](https://am32.ca)
which supports web browser based configuration and firmware update.

You can also use a desktop configurator which you can download from here:

[WINDOWS](https://drive.google.com/file/d/16kaPek9umz7fQFunzBeW4pp2LgT6_E5o/view?usp=drive_link)
[LINUX](https://drive.google.com/file/d/1QtSKwp3RT6sncPADsPkmdasGqNIk68HH/view?usp=sharing)

Alternately you can use the [Online-ESC Configurator](https://esc-configurator.com/) to flash or change settings with any web browser that supports web serial.



## Hardware
AM32 currently has support for STSPIN32F0, STM32F051, STM32G071, GD32E230, AT32F415 and AT32F421.
The CKS32F051 is not recommended due to too many random issues.
Target compatibility List can be found [here](https://github.com/am32-firmware/AM32/blob/main/Inc/targets.h)


## Installation & Bootloader

To use AM32 firmware on a blank ESC, a bootloader must first be installed using an ST-LINK, GD-LINK , CMIS-DAP or AT-LINK.  THe bootloader will be dependant on the MCU used ont he esc . Choose the bootloader that matches the MCU type and signal input pin of the ESC.
The compatibility chart has the bootloader pinouts listed.
Current bootloaders can be found [here](https://github.com/am32-firmware/AM32-bootloader).

After the bootloader has been installed the main firmware from can be installed either with the configuration tools and a Betaflight flight controller or a direct connection with a usb serial adapter modified for one wire.

To update an existing AM32 bootloader an update tool can be found [here](https://github.com/am32-firmware/AM32-unlocker).

## Support and Developers Channel
There are two ways you can get support or participate in improving am32.
We have a discord server here:

https://discord.gg/h7ddYMmEVV

Etiquette: Please wait around long enough for a reply - sometimes people are out flying, asleep or at work and can't answer immediately. 

If you wish to support the project please join the Patreon.

https://www.patreon.com/user?u=44228479


## Sponsors
The AM32 project would not have made this far without help from the following sponsors:

Holmes Hobbies - https://holmeshobbies.com/ - The project would not be where it is today without the support of HH. Check out the Crawlmaster V2 for the best am32 experience!

Repeat Robotics - https://repeat-robotics.com/ - Bringing Am32 esc's to the fighting robot community!

Quaternium - https://www.quaternium.com/ - Firmware development support and hardware donations

Airbot - Many hardware donations

NeutronRC - For hardware, am32 promotion and schematics 

Aikon - Hardware donations and schematics\
Skystars  - For hardware and taking a chance on the first commercial am32 esc's\
Diatone - Hardware donations\
T-motor - Motor and Hardware donations\
HLGRC  - Hardaware donations


## Contributors
A big thanks to all those who contributed time, advice and code to the AM32 project.\
Un!t\
Hugo Chiang (Dusking)\
Micheal Keller (Mikeller)\
ColinNiu\
Jacob Walser

And for feedback from pilots and drivers:\
Jye Smith\
Markus Gritsch\
Voodoobrew

(and many more)


2026-11-21 参数附件基于板上完整读回配置，仅将方向字节从1恢复为0。反向关闭，自动进角 raw0（手动进角），电流限制关闭，Ramp rate raw160（16.0%/ms）。M2 仍使用三秒控制器。参数文件为 M2_SETTINGS_FORWARD_20261121.bin/.hex。
