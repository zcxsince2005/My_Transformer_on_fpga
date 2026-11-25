from __future__ import annotations

import argparse
from pathlib import Path

import torch

from sample_tiny_gpt import generate_reply, load_model
from tokenizer import AsciiTokenizer


def print_help() -> None:
    print("Commands:")
    print("  help          show this help")
    print("  exit          quit")
    print("  len 50        set generated token count")
    print("  len 100")
    print("  len 128")
    print("  len 200")
    print("  temp 0.7      set sampling temperature")
    print("  topk 40       set top-k sampling")
    print("Try prompts: hello, what is fpga, who are you, tell me about ai, what is zynq")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--checkpoint", default="checkpoints/tiny_gpt.pt")
    parser.add_argument("--device", default="cuda" if torch.cuda.is_available() else "cpu")
    parser.add_argument("--max-new-tokens", type=int, default=100)
    parser.add_argument("--temperature", type=float, default=0.7)
    parser.add_argument("--top-k", type=int, default=40)
    args = parser.parse_args()

    here = Path(__file__).resolve().parent
    ckpt = (here / args.checkpoint).resolve()
    model, cfg, _ = load_model(ckpt, args.device)
    tok = AsciiTokenizer(cfg.vocab_size)

    max_new_tokens = max(1, min(args.max_new_tokens, cfg.max_supported_new_tokens))
    temperature = args.temperature
    top_k = args.top_k

    print("Tiny GPT terminal chat")
    print(f"checkpoint: {ckpt}")
    print(f"device: {args.device}")
    print("Type help for commands, exit to quit.")

    while True:
        try:
            user = input("User: ").strip()
        except (EOFError, KeyboardInterrupt):
            print()
            break
        if not user:
            continue
        low = user.lower()
        if low == "exit":
            break
        if low == "help":
            print_help()
            continue
        if low.startswith("len "):
            try:
                value = int(low.split()[1])
                max_new_tokens = max(1, min(value, cfg.max_supported_new_tokens))
                print(f"max_new_tokens = {max_new_tokens}")
            except Exception:
                print("Invalid len command. Example: len 100")
            continue
        if low.startswith("temp "):
            try:
                temperature = max(0.1, min(float(low.split()[1]), 2.0))
                print(f"temperature = {temperature}")
            except Exception:
                print("Invalid temp command. Example: temp 0.7")
            continue
        if low.startswith("topk "):
            try:
                top_k = max(1, min(int(low.split()[1]), cfg.vocab_size))
                print(f"top_k = {top_k}")
            except Exception:
                print("Invalid topk command. Example: topk 40")
            continue

        raw, reply, fallback_used, elapsed = generate_reply(
            model,
            cfg,
            tok,
            user,
            max_new_tokens=max_new_tokens,
            temperature=temperature,
            top_k=top_k,
            device=args.device,
        )
        print("GPT:", reply)
        if fallback_used:
            print("[fallback used]")
        print(f"tokens={max_new_tokens}, avg_ms/token={(elapsed * 1000.0) / max(1, max_new_tokens):.3f}")


if __name__ == "__main__":
    main()
