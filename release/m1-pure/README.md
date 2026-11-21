> 2026-11-21 已撤回此 PURE 发布。用户实机报告启动成功率下降。M1 改用保留11月11日诊断代码的构建，入口为 python3 tools/scripts/build_m1.py。以下为历史归档说明。

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

