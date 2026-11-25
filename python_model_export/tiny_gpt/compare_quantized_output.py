from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
import torch

from export_tiny_gpt_int8 import quantize
from tiny_gpt_config import DEFAULT_CONFIG
from tokenizer import AsciiTokenizer
from train_tiny_gpt import TinyGPT


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--checkpoint", default="checkpoints/tiny_gpt.pt")
    parser.add_argument("--prompt", default="User: fpga\nAI:")
    args = parser.parse_args()

    here = Path(__file__).resolve().parent
    model = TinyGPT(DEFAULT_CONFIG)
    ckpt = (here / args.checkpoint).resolve()
    if ckpt.exists():
        model.load_state_dict(torch.load(ckpt, map_location="cpu")["model"])
    model.eval()
    tok = AsciiTokenizer(DEFAULT_CONFIG.vocab_size)
    idx = torch.tensor([tok.encode(args.prompt)[-DEFAULT_CONFIG.block_size :]], dtype=torch.long)
    with torch.no_grad():
        logits, _ = model(idx)
    fp = logits[0, -1].detach().numpy()
    q, scale = quantize(fp)
    deq = q.astype(np.float32) * scale
    print("prompt:", repr(args.prompt))
    print("fp32_argmax:", int(np.argmax(fp)))
    print("int8_dequant_argmax:", int(np.argmax(deq)))
    print("max_abs_error:", float(np.max(np.abs(fp - deq))))
    print("mean_abs_error:", float(np.mean(np.abs(fp - deq))))


if __name__ == "__main__":
    main()
