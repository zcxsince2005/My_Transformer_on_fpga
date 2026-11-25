# V4 GPT HLS Optimization

## Goal

V4 keeps the stable small GPT model and increases HLS Linear throughput.

The goal is to improve the hardware-accelerated path without using the older approximate token-step path that produced unreadable output.

## Stable Runtime Mode

Default mode:

- `LINEAR_ROLLING_KV`

This means:

- PS still owns tokenizer, sampling, LayerNorm/attention orchestration, and output text.
- PL accelerates the repeated INT8 Linear/matmul kernels.
- The generated text remains aligned with the exported Python model path.

## HLS Changes

Files:

- `HLS\gpt_accel\gpt_accel.h`
- `HLS\gpt_accel\gpt_accel.cpp`
- `vitis2024.2\src\gpt_hw_accel.h`
- `vitis2024.2\src\gpt_hw_accel.c`

Key parameters:

- `GPT_LINEAR_PARALLEL_FACTOR = 64`
- `GPT_MATMUL_OUT_PARALLEL_FACTOR = 4`

The matmul kernel now computes four output columns in one call while each column uses 64 INT8 lanes across the inner dimension.

## Cache Optimization

V4 also reduces PS-side cache maintenance:

- Transposed weight buffers are flushed once during initialization.
- Per-Linear calls no longer flush the immutable weight buffer every time.
- Per-Linear cache maintenance is reduced to the input/output buffers that actually change.

This does not remove all AXI overhead, but it reduces the software-side cost while keeping the same numerical path.

## Validation

C simulation:

- `matmul errors=0`
- `qkv errors=0`
- `mlp errors=0`
- `layernorm errors=0`
- Result: `PASSED`

HLS synthesis/export:

- Result: passed
- Exported IP: `HLS\gpt_accel\gpt_accel_prj\solution1\impl\ip`

GPT HLS resources:

| Resource | V4 |
| --- | ---: |
| DSP | 204 |
| LUT | 67,743 |
| FF | 55,903 |
| BRAM18 | 20 |
| URAM | 1 |

Report:

`result\hls_reports\gpt_accel_csynth_v4.rpt`

## Attempted 8-Output-Lane Build

An 8-output-lane version passed HLS, but Vivado synthesis failed on the host due memory pressure.

V4 therefore ships with 4 output lanes:

- More DSP usage than V2/V3.
- Timing closure achieved.
- Full bitstream generated.

## Vivado Result

PL clock:

- `150 MHz`

Timing:

- Setup WNS: `0.105 ns`
- Hold WHS: `0.010 ns`
- Timing constraints met.

Full routed DSP:

- `1026 / 1728 = 59.38%`

