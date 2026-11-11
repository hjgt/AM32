FD6288 + STM32G431 定制 AM32 版本。主固件采用 30 ms＋300 ms 双矢量定位；启动失败后关断 100 ms，再从第一定位重试。首次之外最多重试 5 次，总共最多 6 轮，启动获取总上限 4.28 s。

| 烧录区域 | BIN 起始地址 | BIN 长度 |
| --- | --- | ---: |
| Bootloader（PB5 V17，与 2026-09-15 发布完全一致） | `0x08000000` | 4096 B |
| 主固件 | `0x08001000` | 59392 B |
| 参数页 | `0x0800F800` | 2048 B |

三段各提供 BIN 和 Intel HEX。HEX 已含绝对地址，烧录时无需添加偏移。BIN 不含地址，须指定表中的起始地址。

参数页与当前板上的备份一致：固定 PWM 24 kHz、正弦启动关闭、启动功率 150%、900 KV、14 极、固定进角 7.5°。运行保护保持原门限，自动重试只用于启动阶段。带载急变螺距尚未验证。

[构建参数与 SHA-256](https://github.com/hjgt/AM32/tree/fd6288-g431-final-2026-11-11/release/fd6288-g431)

在仓库根目录使用 Arm GNU Toolchain 15.2.rel1 构建，确保 `arm-none-eabi-gcc` 在 PATH 上：

```sh
python3 -c 'import json,subprocess; subprocess.run(["make",*json.load(open("release/fd6288-g431/build-args.json"))],check=True)'
```

构建输出目录由 build-args.json 中的 OBJ 指定。主固件构建产物与发布 BIN、HEX 逐字节一致。
