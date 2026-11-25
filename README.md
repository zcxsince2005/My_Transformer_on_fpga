# My_Transformer_on_fpga — 在 XCZU7EV 上运行 nanoGPT / Transformer（V4）

> 本仓库内容为 **`nanogpt_on_fpga_hls_V4`** 工程（nanoGPT INT8 推理 + LeNet-5 数字识别，Zynq UltraScale+ MPSoC 软硬协同实现）。
> 仓库已整体替换为 V4 版本：保留完整源码、HLS/Vivado/Vitis 构建脚本、结果报告，以及可直接上板的 `.bit` / `.xsa` / `.elf`。

---

## 1. 项目概览

本工程把一个字符级 **nanoGPT（因果 Transformer）** 的推理流程映射到 `XCZU7EV` 的 PS（ARM Cortex-A53）+ PL（可编程逻辑）上：

- **PS 端**：Tokenizer、采样、LayerNorm/Attention 调度、UART 菜单与串口交互全部由裸机程序完成。
- **PL 端**：由 Vitis HLS 生成的 INT8 Linear/MatMul 加速 IP（`gpt_accel`），以及 LeNet-5 全连接加速 IP（`lenet_accel`）。
- **附加演示**：LeNet-5 手写数字识别，10 帧测试图全部识别正确（10 / 10）。

默认 GPT 运行模式为 **`LINEAR_ROLLING_KV`**：PS 保留完整 Transformer 控制流，PL 只加速反复调用的 INT8 矩阵乘，因此板上生成文本与 Python 导出模型保持数值一致。

## 2. 软硬件环境

| 项目 | 配置 |
| --- | --- |
| 目标器件 | `xczu7ev-ffvc1156-2-i`（Zynq UltraScale+ MPSoC） |
| PL 时钟 | `150 MHz` |
| 开发工具 | Vivado / Vitis / Vitis HLS `2024.2` |
| 串口 | `115200-8-N-1`，无流控 |
| 参考路径 | 本工程在 `E:\model_on_fpga` 下开发，脚本内工具路径默认为 `G:\Vivado2024.2\...`，换机时只需修改 `scripts\*.bat` 中的 `VIVADO` / `HLS` / `XSCT` 变量 |

## 3. 目录结构

```text
My_Transformer_on_fpga/
├── README.md                     # 本文件
├── docs/                         # V4 设计、训练、导出、上板说明
│   ├── V4_BASELINE.md
│   ├── V4_MODEL_CONFIG.md
│   ├── V4_TINY_GPT_TRAINING.md
│   ├── V4_INT8_EXPORT.md
│   ├── V4_GPT_HLS_OPTIMIZATION.md
│   ├── V4_FINAL_BUILD_SUMMARY.md
│   ├── V4_BOARD_RUN_GUIDE.md
│   └── V4_STAGE0_PATH_CHECK.md
├── python_model_export/          # Python 侧：训练 / 量化 / 权重导出
│   ├── tiny_gpt/                 # 字符级 nanoGPT：数据集、训练、采样、INT8 导出
│   └── lenet/                    # LeNet-5 训练资产、hls4ml 工程与测试图片
├── HLS/                          # Vitis HLS 工程源码
│   ├── gpt_accel/                # INT8 MatMul / QKV / Attention / MLP / LayerNorm 核 + testbench
│   └── lenet_accel/              # LeNet-5 全连接加速核（hls4ml 生成 + 手写 bridge）
├── vivado2024.2/                 # Vivado 工程
│   ├── scripts/                  # create_project / create_bd / build_bitstream / export_xsa
│   ├── ip_repo/                  # HLS 导出并打包好的 IP（gpt / lenet）
│   └── soc_prj/                  # Block Design（.bd/.bda/.xci）与工程文件
├── vitis2024.2/                  # Vitis 裸机应用
│   ├── src/                      # 权威应用源码：菜单、GPT 运行时、硬件加速封装、权重头文件
│   ├── scripts/build_app.tcl     # 应用工程构建脚本
│   └── workspace/ai_menu_app/    # Vitis 应用工程（与 src 同步）
├── result/                       # 结果与交付物
│   ├── hls_reports/              # HLS 综合报告（csynth）
│   ├── vivado_reports/           # 实现后利用率 / 时序 / 功耗报告
│   └── xsa_bit_elf/              # 可直接上板的 bit / xsa / elf 及 PSU 初始化代码
└── scripts/                      # 一键构建与上板脚本
    ├── build_all.bat
    ├── run_hls.bat
    ├── run_vivado.bat
    ├── run_vitis_build.bat
    └── run_main_elf.tcl          # JTAG 下载并运行主程序
```

## 4. 模型与量化配置

V4 回到更小的 GPT 形状，把省下来的资源用于 HLS 并行度：

```text
模型:            character-level nanoGPT / causal Transformer
数据集:          tiny Shakespeare + 对话语料 (data/train.txt, data/val.txt)
参数:            260,736
vocab_size:      128
block_size:      128
n_embd:          96
n_head:          4
head_size:       24
n_layer:         2
mlp_hidden:      384
max_new_tokens:  128  (最大支持 200)
量化:            INT8（逐层 dequant scale）
```

训练与量化校验：

| 指标 | 数值 |
| --- | --- |
| 训练设备 | CUDA（RTX 5060 Laptop） |
| 迭代次数 / batch | 6,000 / 32 |
| 最终 train loss | 1.0367 |
| 最终 val loss | 1.0283 |
| FP32 argmax / INT8 argmax | 32 / 32（一致） |
| 量化最大绝对误差 | 0.05365 |
| 量化平均绝对误差 | 0.02704 |

## 5. 硬件加速设计

- `GPT_LINEAR_PARALLEL_FACTOR = 64`：内层 INT8 MAC 并行度 64。
- `GPT_MATMUL_OUT_PARALLEL_FACTOR = 4`：单次 IP 调用计算 4 个输出列。
- 权重转置缓冲在初始化时一次性 flush，逐次 Linear 调用只维护输入/输出缓冲，降低 PS 侧 cache 维护开销。
- HLS C 仿真：`matmul / qkv / mlp / layernorm` 全部 `errors=0`，结果 `PASSED`。
- 曾尝试 8 输出通道版本：HLS 通过，但 Vivado 综合因主机内存不足失败，因此 V4 采用 4 输出通道。

HLS 资源（`result/hls_reports/gpt_accel_csynth_v4.rpt`）：

| BRAM18 | DSP | FF | LUT | URAM |
| ---: | ---: | ---: | ---: | ---: |
| 20 | 204 | 55,903 | 67,743 | 1 |

## 6. 实现结果（Vivado，150 MHz）

| 资源 | 使用 / 总量 | 占比 |
| --- | ---: | ---: |
| LUT | 117,328 / 230,400 | 50.92% |
| FF | 130,452 / 460,800 | 28.31% |
| BRAM Tile | 61 / 312 | 19.55% |
| URAM | 1 / 96 | 1.04% |
| DSP | 1,026 / 1,728 | 59.38% |

时序：Setup WNS `0.105 ns`，Hold WHS `0.010 ns`，**时序收敛**。

报告：`result/vivado_reports/utilization_routed.rpt`、`timing_summary_routed.rpt`、`power_routed.rpt`。

## 7. 板级运行

1. 断电重启开发板，关闭 Vivado Hardware Manager / 旧 XSCT / 占用串口的程序。
2. 打开串口：`115200-8-N-1`。
3. 下载并运行：

```bat
xsct.bat scripts\run_main_elf.tcl
:: 或等效兼容命令
xsct.bat scripts\jtag_run_test.tcl full-init
```

4. 串口菜单：

```text
UART INIT DONE
LeNet + tiny GPT FPGA demo

Menu:
1. Run digit recognition demo
2. Print digit recognition performance
3. Enter tiny GPT dialogue
4. Print last tiny GPT generation performance
5. Print FPGA resource utilization
```

预期结果：

- 选项 1：`accuracy = 10 / 10`。
- 选项 3：输入 `hello` / `what is fpga` / `why use gpu` 可得到可读英文回复（tiny 字符级模型，不是通用助手）。
- 选项 4：显示 `GPT mode: LINEAR_ROLLING_KV`、`n_embd: 96`、`HLS linear fallbacks: 0` 等运行信息与逐 token 时延。
- 选项 5：显示上述 V4 资源与时序数据。

## 8. 从源码重新构建

在 Vivado/Vitis 2024.2 的 Windows 命令行中执行：

```bat
scripts\build_all.bat
```

该脚本依次完成：

1. `run_hls.bat`：导出 INT8 权重头文件 → 同步到 HLS/Vitis 源码 → 运行 LeNet 与 GPT 的 HLS 综合 → 导出 IP 并复制到 `vivado2024.2/ip_repo` → 汇总 HLS 报告。
2. `run_vivado.bat`：创建工程 → 生成 Block Design → 综合/实现/生成 bitstream → 导出 XSA。
3. `run_vitis_build.bat`：基于新 XSA 构建裸机应用，生成 ELF。

## 9. 交付物

| 文件 | 说明 |
| --- | --- |
| `result/xsa_bit_elf/nanogpt_zynqmp.bit` | PL 配置文件 |
| `result/xsa_bit_elf/nanogpt_zynqmp.xsa` | 硬件平台（含 bit） |
| `result/xsa_bit_elf/nanogpt_ps_app.elf` | PS 裸机应用 |
| `result/xsa_bit_elf/gpt_accel_csynth.rpt` | GPT IP 综合报告 |
| `result/xsa_bit_elf/vivado_*.rpt` | 实现后利用率 / 时序 / 功耗报告 |
| `result/xsa_bit_elf/address_map.txt` | AXI 地址映射 |
| `result/xsa_bit_elf/board_run_steps.txt` | 上板步骤 |

## 10. 关于构建产物

为保证仓库可克隆、可维护，**不纳入版本库**的内容包括（均可用脚本重新生成）：

- HLS 工程求解目录 `HLS/*/*_prj/solution1/`（`.autopilot` 数据库、RTL、综合/实现中间结果）；
- Vivado 自动生成目录 `vivado2024.2/soc_prj/nanogpt_soc.{gen,cache,hw,ip_user_files,sim,runs}/`；
- Vitis 平台构建目录 `vitis2024.2/workspace/nanogpt_platform/`；
- 实现中间文件 `result/vivado_reports/*.dcp`（布局/布线数据库，单个 20–250 MB）。

规则见 `.gitignore`。已预置的 `vivado2024.2/ip_repo`、`result/hls_reports`、`result/vivado_reports/*.rpt` 与 `result/xsa_bit_elf` 可直接使用，无需重新综合。

## 11. 文档索引

| 文档 | 内容 |
| --- | --- |
| [`docs/V4_BASELINE.md`](docs/V4_BASELINE.md) | V4 基线、设计取舍与后续优化方向 |
| [`docs/V4_MODEL_CONFIG.md`](docs/V4_MODEL_CONFIG.md) | 模型形状与选型理由 |
| [`docs/V4_TINY_GPT_TRAINING.md`](docs/V4_TINY_GPT_TRAINING.md) | 训练数据、命令与结果 |
| [`docs/V4_INT8_EXPORT.md`](docs/V4_INT8_EXPORT.md) | INT8 量化导出流程与误差 |
| [`docs/V4_GPT_HLS_OPTIMIZATION.md`](docs/V4_GPT_HLS_OPTIMIZATION.md) | HLS 并行度与 cache 优化细节 |
| [`docs/V4_FINAL_BUILD_SUMMARY.md`](docs/V4_FINAL_BUILD_SUMMARY.md) | 最终构建汇总（资源、时序、交付物） |
| [`docs/V4_BOARD_RUN_GUIDE.md`](docs/V4_BOARD_RUN_GUIDE.md) | 上板与串口验证步骤 |
| [`docs/V4_STAGE0_PATH_CHECK.md`](docs/V4_STAGE0_PATH_CHECK.md) | 路径独立性与脚本检查记录 |

## 12. 后续工作

当前瓶颈是 **PS/PL 调用粒度**——每个 Linear 都要单独发起一次 IP 调用。下一步计划实现数值一致的融合算子：

- fused QKV projection；
- fused MLP；
- 已验证的 token-step 快速路径（需与当前稳定路径 logits 对齐）。

---

如有问题或建议，欢迎提 Issue。
