# V4 INT8 Export

Working directory:

`E:\model_on_fpga\nanogpt_on_fpga_hls_V4\python_model_export\tiny_gpt`

Command:

```powershell
python export_tiny_gpt_int8.py
```

Generated headers:

- `exported\gpt_config.h`
- `exported\gpt_vocab.h`
- `exported\gpt_weights.h`
- `exported\gpt_test_vectors.h`

Synchronized to:

- `HLS\gpt_accel\gpt_config.h`
- `HLS\gpt_accel\gpt_vocab.h`
- `HLS\gpt_accel\gpt_weights.h`
- `HLS\gpt_accel\gpt_test_vectors.h`
- `vitis2024.2\src\gpt_config.h`
- `vitis2024.2\src\gpt_vocab.h`
- `vitis2024.2\src\gpt_weights.h`
- `vitis2024.2\src\gpt_test_vectors.h`

Exported config:

- `GPT_VOCAB_SIZE = 128`
- `GPT_BLOCK_SIZE = 128`
- `GPT_N_EMBD = 96`
- `GPT_N_HEAD = 4`
- `GPT_N_LAYER = 2`
- `GPT_HEAD_SIZE = 24`
- `GPT_MLP_HIDDEN = 384`
- `GPT_MAX_NEW_TOKENS = 128`
- `GPT_MAX_SUPPORTED_NEW_TOKENS = 200`

INT8 comparison command:

```powershell
python compare_quantized_output.py --prompt "User: what is fpga`nAssistant:"
```

Result:

- `fp32_argmax = 32`
- `int8_dequant_argmax = 32`
- `max_abs_error = 0.05365180969238281`
- `mean_abs_error = 0.02704065851867199`

Runtime synchronization:

- `vitis2024.2\src\gpt_runtime.c` dequantization scale constants were updated to match the V4 `gpt_weights.h`.

