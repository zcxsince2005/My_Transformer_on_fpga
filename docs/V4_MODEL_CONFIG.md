# V4 Model Configuration

V4 returns to the smaller GPT model used by the stable V2-style path.

Configuration:

- `vocab_size = 128`
- `block_size = 128`
- `n_embd = 96`
- `n_head = 4`
- `head_size = 24`
- `n_layer = 2`
- `mlp_hidden = 384`
- `max_new_tokens = 128`
- `max_supported_new_tokens = 200`

Reasoning:

- The smaller model reduces per-token compute.
- HLS optimization should spend FPGA resources on parallelism instead of making the model larger.
- This keeps the board-side text path aligned with the Python-exported model.
- V4 keeps `LINEAR_ROLLING_KV` as the stable default GPT path.

Expected impact:

- Better speed potential than the larger V3 model.
- More GPT DSP usage than V2/V3 because HLS matmul output lanes are widened.
- Final speed must be measured on board with menu option 4.

