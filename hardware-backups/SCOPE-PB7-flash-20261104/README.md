# ALIGN-PAIR + PB7 实验资料索引

2026-11-05整理。接手先读 [最新交接](../../docs/HANDOFF-20261105.md)，版本机制见 [PB7说明](../../docs/FD6288-SCOPE-PB7-20261104.md)。本目录是已完成采集的证据，不含拟议的“只定位、不发车”实验结果。

## 样本对应

接管时间从试验启动计，约90ms为首次发车。trial仅标识各份采集中的本次试验，不跨会话累计。

| 样本 | 测量相 / trial | 照片 | 采集JSON | 结果 |
|---|---|---|---|---|
| 普通成功 | A / 1 | [3431](scope-success/IMG_3431.png)、[3432](scope-success/IMG_3432.png) | [capture-success](capture-success.bin.json) | 首扇区30ms超时，11次超时推进，约335ms接管；[说明](RESULT-success.md) |
| 顺滑成功 | A / 32 | [3433](scope-success/IMG_3433.png) | [capture-smooth](capture-smooth.bin.json) | 首扇区约6.86ms，6次连续接受，0超时，约136ms接管；[说明](RESULT-smooth.md) |
| 失败 | A / 3 | [3434](scope-failure/IMG_3434.png)、[3435](scope-failure/IMG_3435.png)、[3436](scope-failure/IMG_3436.png)（定位补图） | [capture-failure](capture-failure.bin.json) | 0接受、31次超时，约820ms无锁定停机；[对照说明](RESULT-failure-comparison.md) |
| 失败 | B / 25 | [3437](scope-failure/IMG_3437.png) | [capture-b-failure](capture-b-failure.bin.json) | 首扇区30ms超时，总3个孤立接受，约820ms无锁定停机；[说明](RESULT-b-failure.md) |
| 顺滑成功 | B / 14 | [3438](scope-success/IMG_3438.png) | [capture-b-smooth](capture-b-smooth.bin.json) | 首扇区约12.24ms，6次连续接受，0超时，约141ms接管；[成败对照](RESULT-b-smooth.md) |

每份JSON另有同名 `.bin`。这些小BIN不是完整RAM镜像；完整分析应保留JSON及匹配ELF，不能只凭BIN大小判断证据是否完整。失败样本的详细扇区缓冲仅保留前24个，后续只据汇总说明，不能重构缺失扇区。

**B相这对样本均为自由放置**，已由用户后续明确。A相各次机械放置条件未在本索引补作推断。

## 版本与读取依据

- [flash.json](flash.json)：已有写入及运行命令记录；[flash.py](flash.py)为当时脚本。
- [HEX / ELF / BIN及哈希](../../test-firmware/fd6288-SCOPE-PB7-20261104/)；[构建参数](../../docs/scope-pb7-20261104/on-args.json)。
- 各RESULT记录了当次归零门禁、代码身份及冻结缓冲复读检查。此处未重新连接硬件或重采RAM。
- 照片为用户HEIC转换的PNG。软件margin、浮空相端电压和转子机械位置是不同证据，不能互相替代。

## 保留的结论

前侧保护保留时也可顺滑起转；B相成败差异在定位末段已经出现。固定定位末状态不一致是优先验证假设，未确认根因。第二定位已为60ms；后续隔离实验见 [诊断计划](../../docs/FD6288-ALIGN-ONLY-PLAN-20261105.md)。
