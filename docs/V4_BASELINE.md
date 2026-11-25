# nanogpt_on_fpga_hls_V4 Baseline

V4 is based on the stable V3 project, but it returns GPT model size to the smaller 96-wide configuration and spends additional FPGA resources on HLS Linear acceleration.

## Stable Functional Baseline

Expected board behavior:

- LeNet menu option 1: 10 / 10 digit accuracy.
- GPT menu option 3: readable tiny-GPT replies for core prompts.
- GPT menu option 4: performance report.
- Resource menu option 5: V4 resource report.

## Design Choice

V4 does not use the older approximate token-step path as the default path, because that path previously produced unreadable responses.

V4 default:

- `LINEAR_ROLLING_KV`
- PS keeps full Transformer control flow.
- PL accelerates INT8 Linear/matmul calls.

## V4 Optimization Compared With Earlier Stable Path

Model:

- `n_embd = 96`
- `n_head = 4`
- `n_layer = 2`
- `mlp_hidden = 384`

HLS:

- Inner MAC parallel factor: `64`
- Output-column parallel factor: `4`
- GPT HLS DSP: `204`

Vivado:

- PL clock: `150 MHz`
- Full routed DSP: `1026 / 1728`
- Timing met.

## Remaining Bottleneck

The main remaining bottleneck is still PS/PL call granularity.

Future optimization should implement a numerically matched fused block such as:

- fused QKV projection;
- fused MLP;
- eventually a verified token-step that produces the same logits as the current stable path.

