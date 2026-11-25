# tiny_gpt Python 目录说明

本目录是 tiny GPT 的 Python 端工程，当前只负责：

- 准备训练数据；
- 训练 tiny GPT 模型；
- 在终端中验证模型对话效果；
- 后续导出 INT8 C 头文件。

本目录不会自动构建 FPGA bitstream，也不会重新生成 XSA 或 Vitis ELF。

## 目录结构

```text
tiny_gpt
├─ data
├─ checkpoints
├─ exported
├─ prepare_dataset.py
├─ tiny_gpt_config.py
├─ tokenizer.py
├─ train_tiny_gpt.py
├─ sample_tiny_gpt.py
├─ chat_tiny_gpt.py
├─ export_tiny_gpt_int8.py
└─ compare_quantized_output.py
```

## data 目录

```text
data/train.txt
```

训练集。`train_tiny_gpt.py` 默认读取这个文件进行训练。

```text
data/val.txt
```

验证集。训练过程中用它计算 validation loss，用来判断模型是否过拟合。

```text
data/dataset_info.txt
```

数据集说明文件，记录数据来源、字符数、训练集大小、验证集大小等信息。

当前数据集是偏现代助手问答风格的数据，主要由两部分组成：

- 下载的 Alpaca instruction 数据；
- 本地生成的 FPGA / AI / Zynq / HLS / LeNet 等技术问答数据。

## checkpoints 目录

```text
checkpoints/tiny_gpt.pt
```

这是训练好的 PyTorch 模型 checkpoint。

它保存了：

- 模型结构配置；
- 模型权重；
- train loss；
- val loss；
- 训练迭代数；
- 参数量；
- 训练耗时。

当前 Python 端对话程序加载的就是这个文件。

## exported 目录

```text
exported/
```

这个目录用于保存后续导出的 C 头文件。当前如果没有执行 INT8 导出，它可以是空目录。

运行：

```powershell
python export_tiny_gpt_int8.py
```

之后会生成：

```text
exported/gpt_config.h
exported/gpt_vocab.h
exported/gpt_weights.h
exported/gpt_test_vectors.h
```

这些文件用于后续同步到 HLS / Vitis / FPGA 工程。

注意：

```text
checkpoints/tiny_gpt.pt
```

才是当前 Python 端正在使用的模型。

```text
exported/*.h
```

只是给后续 C / HLS / Vitis 使用的导出结果。

## Python 文件说明

```text
prepare_dataset.py
```

数据准备脚本。

主要功能：

- 下载 Alpaca instruction 数据；
- 生成 FPGA / AI / Zynq / HLS / LeNet 相关问答样本；
- 清洗文本；
- 生成 `data/train.txt`；
- 生成 `data/val.txt`；
- 生成 `data/dataset_info.txt`。

```text
tiny_gpt_config.py
```

模型配置文件。

包含：

- `vocab_size`
- `block_size`
- `n_embd`
- `n_head`
- `n_layer`
- `dropout`
- 默认训练参数；
- 默认生成 token 数。

```text
tokenizer.py
```

ASCII 字符级 tokenizer。

主要功能：

- 把文本编码成 token id；
- 把 token id 解码成文本；
- 生成 C 端使用的 `gpt_vocab.h`。

当前模型是 char-level GPT，不是 BPE tokenizer，也不是 WordPiece tokenizer。

```text
train_tiny_gpt.py
```

训练脚本，也是模型定义所在文件。

里面包含：

- `TinyGPT`
- `CausalSelfAttention`
- `MLP`
- `Block`
- batch 采样；
- train loss / val loss 评估；
- checkpoint 保存。

训练完成后输出：

```text
checkpoints/tiny_gpt.pt
```

```text
sample_tiny_gpt.py
```

非交互式测试脚本。

用于快速测试一个 prompt，例如：

```powershell
python sample_tiny_gpt.py --prompt "what is fpga"
```

它会输出：

- checkpoint 路径；
- prompt；
- 生成 token 数；
- 平均每 token 耗时；
- 是否触发 fallback；
- 模型生成文本；
- 原始模型输出文本。

```text
chat_tiny_gpt.py
```

终端交互式聊天脚本。

运行：

```powershell
python chat_tiny_gpt.py --device cuda
```

进入交互后可以输入：

```text
hello
what is fpga
who are you
tell me about ai
what is zynq
```

支持命令：

```text
help
exit
len 100
temp 0.7
topk 40
```

```text
export_tiny_gpt_int8.py
```

INT8 权重导出脚本。

它会读取：

```text
checkpoints/tiny_gpt.pt
```

然后导出：

```text
exported/gpt_config.h
exported/gpt_vocab.h
exported/gpt_weights.h
exported/gpt_test_vectors.h
```

这些文件后续用于 FPGA / Vitis 集成。

```text
compare_quantized_output.py
```

量化对比脚本。

用于比较：

- FP32 logits；
- INT8 量化再反量化后的 logits。

这个脚本主要在后续导出 INT8 权重前后使用，用来检查量化误差。

## 常用命令

进入目录：

```powershell
cd E:\model_on_fpga\nanogpt_on_fpga\python_model_export\tiny_gpt
```

准备数据：

```powershell
python prepare_dataset.py
```

使用 GPU 训练：

```powershell
python train_tiny_gpt.py --device cuda --max-iters 8000 --batch-size 64 --eval-interval 1000 --eval-iters 80
```

测试单个 prompt：

```powershell
python sample_tiny_gpt.py --prompt "what is fpga" --max-new-tokens 120 --temperature 0.55 --top-k 25
```

启动终端聊天：

```powershell
python chat_tiny_gpt.py --device cuda
```

后续导出 INT8 头文件：

```powershell
python export_tiny_gpt_int8.py
```

## 当前边界

当前阶段只处理 Python 端。

不要在本目录中自动执行：

- HLS synthesis；
- Vivado bitstream；
- XSA 导出；
- Vitis ELF 编译。

如果 Python 端模型效果确认可用，下一阶段再执行 INT8 导出，并把新权重同步到 Vitis / HLS 工程。

## 重要说明

这个模型是一个小型 char-level GPT，用于 FPGA 部署演示。

它可以在终端中回答简单问题，但它不是完整的大语言模型。

当前主要目标是：

- 输出可读英文；
- 能回答简单助手类问题；
- 能回答 FPGA / AI / Zynq / HLS 相关基础问题；
- 为后续 INT8 导出和 FPGA 端部署做准备。
