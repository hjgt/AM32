# M1 · FD6288 / STM32G431

M1 主分支为 [m1-main](https://github.com/hjgt/AM32/tree/m1-main)，使用 [2026-11-11 M1 Release](https://github.com/hjgt/AM32/releases/tag/m1-fd6288-g431-final-2026-11-11)。M2 使用独立的 [m2-main](https://github.com/hjgt/AM32/tree/m2-main) 和 M2 Release。

## 构建已验证的 M1

保留11月11日的诊断记录代码。FD6288_M1_PURE=0 为默认值，标准构建入口也显式固定此值。

在仓库根目录运行 python3 tools/scripts/build_m1.py。

使用 Arm GNU Toolchain 15.2.Rel1（GCC 15.2.1）。脚本按 release/fd6288-g431/build-args.json 编译，再核对 BIN 和 HEX 与11月11日发布文件的 SHA-256。任一不一致会报错退出。原参数中的 FD6288_NO_DIAGNOSTICS=1 属于11月11日基线，应沿用整套参数。

2026-11-21 已重新编译确认，恢复诊断代码后 BIN 和 HEX 与11月11日 Release 逐字节完全一致。应用 BIN 的 SHA-256 为 a9808e0890301e39ed930bf47f160127c7a01cf115c40486c7b710f55831939b，HEX 为 7f30d67bda2a7c3d2bdb2dbe59920d83ebe751e252059571f0ead849989a8e2b。

## PURE 撤回记录

用户实机报告 M1 PURE 启动成功率下降，已撤下2026-11-20的 M1 PURE Release。历史 PURE 源码和显式构建入口保留用于追溯，M1 日常构建使用上面的 build_m1.py。

此前主机测试验证的是控制状态和采样调度，无法覆盖诊断写入移除后的 MCU 指令耗时、内存布局和中断时序。保留诊断版恢复了原始机器码，具体成功率差异的物理原因尚未定位。

## 烧录文件

| 区域 | 地址 | 文件长度 |
| --- | --- | --- |
| PB5 V17 Bootloader | 0x08000000 | 4096 B |
| M1应用 | 0x08001000 | 59392 B |
| M1参数页 | 0x0800F800 | 2048 B |

应用 HEX 包含绝对地址。完整三段镜像继续从11月11日 Release 下载，参数页与文件校验值记录在 release/fd6288-g431/。

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

