# M1 PURE · FD6288 / STM32G431

M1 主分支是 [`m1-main`](https://github.com/hjgt/AM32/tree/m1-main)。M2 主分支是 [`m2-main`](https://github.com/hjgt/AM32/tree/m2-main)，历史调试分支继续以 `codex/m1-` 区分。

本版以 [11月11日 M1 Release](https://github.com/hjgt/AM32/releases/tag/m1-fd6288-g431-final-2026-11-11) 为基线，只通过编译开关关闭调试采集和统计；原始记录代码完整保留。最新 M1 PURE Release 对应本分支的发布 tag，构建参数位于 `release/m1-pure/build-args.json`。

## 构建开关

- `FD6288_M1_PURE=1`：关闭启动记录、ADC 原始数据缓冲、定位统计、扇区统计、重试统计及 PB7 示波标记。静态 RAM 为3864B，原发布版为20536B。
- `FD6288_M1_PURE=0`：恢复原诊断构建。在11月11日构建参数下，BIN、HEX与原发布文件逐字节一致。
- 保留原30ms＋300ms双矢量定位、关断100ms后重试、最多6次尝试、总4.28s获取上限、真实反馈六步接管及原有运行保护。
- 定位期间600/900tick ADC触发点切换及512组调度预算保持原样。它会影响真实采样时序，PURE仅移除数据存储；8B状态用于保留此时序。DWT时钟初始化也是控制所需，保留。

```sh
python3 tools/scripts/build_m1_pure.py
node tests/run_fd6288_m1_pure_tests.cjs
node tests/check_fd6288_m1_pure_image.cjs obj-fd6288-m1-pure-20261120/AM32_CUSTOM_FD6288_G431_2.20.elf obj-fd6288-m1-pure-20261120/AM32_CUSTOM_FD6288_G431_2.20.bin
```

PURE/诊断两种实际C控制路径通过同一组主机测试：双向和计时回绕、各轮启动成功、各位置失反馈、重试上限、关断间隔、接管、归零、解锁状态变化和原生故障；控制轨迹校验值一致。实际ADC定位触发时序也逐组一致。测试使用ASan/UBSan；本次没有烧录，PURE实机复测待完成，因此以Pre-release归档。

## 完整三段归档

| 区域 | BIN地址 | 长度 | 来源 |
|---|---|---:|---|
| Bootloader | `0x08000000` | 4096B | 11月11日PB5 V17，逐字节复制 |
| M1 PURE应用 | `0x08001000` | 59392B | 从本分支最新发布提交构建 |
| M1参数页 | `0x0800f800` | 2048B | 11月11日M1参数页，逐字节复制 |

各段提供BIN和HEX。HEX带绝对地址；BIN须指定表中地址。参数保持M1原设置，未使用M2的0.1%参数页。Bootloader与参数的源文件、哈希保存在 `release/m1-pure/`。固件文件标识为 `M1_PURE_FD6288_G431`。应用本身不包含Bootloader和参数页，它们作为独立附件提供。

发布清单记录源码提交、构建参数和每个附件的SHA-256。11月11日历史tag与附件保留。

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

