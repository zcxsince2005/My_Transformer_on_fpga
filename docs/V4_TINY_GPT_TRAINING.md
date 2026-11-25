# V4 Tiny GPT Training

Working directory:

`E:\model_on_fpga\nanogpt_on_fpga_hls_V4\python_model_export\tiny_gpt`

Dataset:

- `data\train.txt`: 10,434,286 encoded characters
- `data\val.txt`: 431,435 encoded characters
- Existing conversational dataset was reused.

Command:

```powershell
python train_tiny_gpt.py --device cuda --max-iters 6000 --batch-size 32 --eval-interval 500 --eval-iters 50
```

Training result:

- Device: `cuda`
- GPU: `NVIDIA GeForce RTX 5060 Laptop GPU`
- Parameters: `260,736`
- Max iterations: `6000`
- Final train loss: `1.0367`
- Final val loss: `1.0283`
- Checkpoint: `checkpoints\tiny_gpt.pt`

Python sampling checks:

```powershell
python sample_tiny_gpt.py --prompt "hello" --max-new-tokens 80 --temperature 0.55 --top-k 25
python sample_tiny_gpt.py --prompt "what is fpga" --max-new-tokens 80 --temperature 0.55 --top-k 25
python sample_tiny_gpt.py --prompt "who are you" --max-new-tokens 80 --temperature 0.55 --top-k 25
python sample_tiny_gpt.py --prompt "why use gpu" --max-new-tokens 80 --temperature 0.55 --top-k 25
```

Observed outputs were readable for the core demo prompts, but this remains a tiny character-level model rather than a general assistant.

Board-side validation must use menu option 3 and then option 4 for timing.

