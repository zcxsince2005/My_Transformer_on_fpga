# V4 Final Build Summary

Project:

`E:\model_on_fpga\nanogpt_on_fpga_hls_V4`

## Scope

- V4 was copied from the stable V3 baseline.
- V1/V2/V3/PS_V1/my_demo were not modified.
- V4 keeps the stable GPT runtime path and focuses on wider HLS Linear acceleration.

## Model

V4 returns to the small GPT shape:

- `vocab_size = 128`
- `block_size = 128`
- `n_embd = 96`
- `n_head = 4`
- `head_size = 24`
- `n_layer = 2`
- `mlp_hidden = 384`
- `max_new_tokens = 128`

Training:

- Device: CUDA
- Parameters: `260,736`
- Iterations: `6000`
- Final train loss: `1.0367`
- Final val loss: `1.0283`
- Checkpoint: `python_model_export\tiny_gpt\checkpoints\tiny_gpt.pt`

## INT8 Export

Headers were regenerated and copied to both HLS and Vitis source trees:

- `gpt_config.h`
- `gpt_vocab.h`
- `gpt_weights.h`
- `gpt_test_vectors.h`

Quantized comparison:

- FP32 argmax: `32`
- INT8 dequant argmax: `32`
- Max absolute error: `0.05365180969238281`
- Mean absolute error: `0.02704065851867199`

## GPT HLS Optimization

Stable runtime mode:

- `LINEAR_ROLLING_KV`
- Token-step/block-step approximate paths are not used as the default path.

HLS changes:

- Inner INT8 MAC parallel factor: `64`
- Matmul output columns per IP call: `4`
- Weight transpose buffers are flushed once during initialization.
- Per-Linear cache maintenance is reduced from the older per-call flush pattern.

An 8-output-lane version passed HLS but failed Vivado synthesis due host memory pressure, so V4 ships with 4 output lanes.

GPT HLS resource report:

- BRAM18: `20`
- DSP: `204`
- FF: `55,903`
- LUT: `67,743`
- URAM: `1`

Report:

`result\hls_reports\gpt_accel_csynth_v4.rpt`

## Vivado

PL clock:

- `150 MHz`

The PL clock was lowered from the earlier higher setting so the widened GPT IP could route with timing closure.

Routed full-design resources:

- LUT: `117,328 / 230,400 = 50.92%`
- FF: `130,452 / 460,800 = 28.31%`
- BRAM Tile: `61 / 312 = 19.55%`
- URAM: `1 / 96 = 1.04%`
- DSP: `1,026 / 1,728 = 59.38%`

Timing:

- Setup WNS: `0.105 ns`
- Hold WHS: `0.010 ns`
- Timing constraints: met

Reports:

- `result\vivado_reports\utilization_routed.rpt`
- `result\vivado_reports\timing_summary_routed.rpt`
- `result\vivado_reports\power_routed.rpt`

## Vitis

Final application ELF:

`result\xsa_bit_elf\nanogpt_ps_app.elf`

The app ELF was rebuilt after:

- New XSA export
- New GPT INT8 headers
- New runtime scale table
- Updated HLS/Vivado resource values

## Final Artifacts

Main deliverables:

- `result\xsa_bit_elf\nanogpt_zynqmp.bit`
- `result\xsa_bit_elf\nanogpt_zynqmp.xsa`
- `result\xsa_bit_elf\nanogpt_ps_app.elf`
- `result\xsa_bit_elf\gpt_accel_csynth.rpt`
- `result\xsa_bit_elf\vivado_utilization_routed.rpt`
- `result\xsa_bit_elf\vivado_timing_summary_routed.rpt`
- `result\xsa_bit_elf\vivado_power_routed.rpt`
