from __future__ import annotations

import argparse
import re
import time
from dataclasses import fields
from pathlib import Path

import torch

from tiny_gpt_config import DEFAULT_CONFIG, TinyGPTConfig
from tokenizer import AsciiTokenizer
from train_tiny_gpt import TinyGPT


FALLBACKS = {
    "hello": "hello, I am a tiny fpga gpt demo.",
    "hi": "hi, nice to meet you.",
    "what is fpga": "fpga is a programmable chip for hardware acceleration.",
    "explain fpga": "fpga is a programmable chip for hardware acceleration.",
    "who are you": "I am a tiny gpt model running on zynq mpsoc.",
    "tell me about ai": "ai helps machines learn from data.",
    "what is ai": "ai helps machines learn from data.",
    "what is zynq": "zynq combines arm processors and fpga logic.",
    "what is hls": "hls converts c code into hardware logic.",
}

KEYWORDS = {
    "fpga": ("fpga", "programmable", "hardware", "chip", "logic", "acceleration"),
    "ai": ("ai", "data", "learn", "model", "machine"),
    "zynq": ("zynq", "arm", "fpga", "processor", "logic"),
    "hls": ("hls", "c", "hardware", "logic", "code"),
}


def config_from_checkpoint(obj: dict) -> TinyGPTConfig:
    cfg_dict = dict(DEFAULT_CONFIG.__dict__)
    cfg_dict.update(obj.get("config", {}))
    valid = {f.name for f in fields(TinyGPTConfig)}
    return TinyGPTConfig(**{k: v for k, v in cfg_dict.items() if k in valid})


def load_model(checkpoint: Path, device: str) -> tuple[TinyGPT, TinyGPTConfig, dict]:
    if not checkpoint.exists():
        raise FileNotFoundError(f"checkpoint not found: {checkpoint}")
    obj = torch.load(checkpoint, map_location=device)
    cfg = config_from_checkpoint(obj)
    model = TinyGPT(cfg).to(device)
    model.load_state_dict(obj["model"])
    model.eval()
    return model, cfg, obj


def normalize_prompt(prompt: str) -> str:
    return prompt.strip()


def model_prompt(user_prompt: str) -> str:
    return f"User: {normalize_prompt(user_prompt)}\nAssistant:"


def safe_decode(text: str) -> str:
    text = "".join(ch if (ch == "\n" or 32 <= ord(ch) <= 126) else " " for ch in text)
    text = text.replace("\r", "\n").replace("\t", " ")
    text = re.sub(r"[ ]+", " ", text)
    text = re.sub(r"\n{3,}", "\n\n", text)
    return text.strip()


def clean_completion(text: str) -> str:
    text = safe_decode(text)
    stop_markers = [
        "\nUser:",
        "\nHuman:",
        "\nQuestion:",
        "\nInstruction:",
        "\nSystem:",
        "\nResponse:",
        "\nAnswer:",
        "\nAI:",
    ]
    cut = len(text)
    for marker in stop_markers:
        pos = text.find(marker)
        if pos >= 0:
            cut = min(cut, pos)
    text = text[:cut].strip()
    if text.lower().startswith("assistant:"):
        text = text[len("assistant:") :].strip()
    return text


def fallback_for(prompt: str) -> str:
    p = prompt.lower().strip()
    for key, value in FALLBACKS.items():
        if key in p:
            return value
    return "I am a tiny gpt demo. I can answer short questions about fpga, ai, zynq, and hls."


def looks_readable(prompt: str, text: str) -> bool:
    if len(text) < 12:
        return False
    printable = sum(1 for ch in text if ch == "\n" or 32 <= ord(ch) <= 126)
    if printable / max(1, len(text)) < 0.98:
        return False
    letters = sum(ch.isalpha() for ch in text)
    if letters < 8:
        return False
    if re.search(r"([:;,.!?])\1{3,}", text):
        return False
    if re.search(r"([A-Za-z])\1{5,}", text):
        return False
    low_prompt = prompt.lower()
    low_text = text.lower()
    for topic, words in KEYWORDS.items():
        if topic in low_prompt and not any(word in low_text for word in words):
            return False
    return True


@torch.no_grad()
def generate_reply(
    model: TinyGPT,
    cfg: TinyGPTConfig,
    tokenizer: AsciiTokenizer,
    prompt: str,
    max_new_tokens: int,
    temperature: float,
    top_k: int,
    device: str,
) -> tuple[str, str, bool, float]:
    prompt_text = model_prompt(prompt)
    ids = tokenizer.encode(prompt_text)
    x = torch.tensor([ids[-cfg.block_size :]], dtype=torch.long, device=device)
    t0 = time.perf_counter()
    y = model.generate(x, max_new_tokens=max_new_tokens, temperature=temperature, top_k=top_k)[0].tolist()
    t1 = time.perf_counter()
    new_ids = y[len(x[0]) :]
    raw = tokenizer.decode(new_ids)
    reply = clean_completion(raw)
    fallback_used = False
    if not looks_readable(prompt, reply):
        reply = fallback_for(prompt)
        fallback_used = True
    return raw, reply, fallback_used, t1 - t0


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--checkpoint", default="checkpoints/tiny_gpt.pt")
    parser.add_argument("--prompt", default="hello")
    parser.add_argument("--max-new-tokens", type=int, default=100)
    parser.add_argument("--temperature", type=float, default=0.7)
    parser.add_argument("--top-k", type=int, default=40)
    parser.add_argument("--device", default="cuda" if torch.cuda.is_available() else "cpu")
    args = parser.parse_args()

    here = Path(__file__).resolve().parent
    ckpt = (here / args.checkpoint).resolve()
    model, cfg, obj = load_model(ckpt, args.device)
    tok = AsciiTokenizer(cfg.vocab_size)
    raw, reply, fallback_used, elapsed = generate_reply(
        model,
        cfg,
        tok,
        args.prompt,
        max_new_tokens=max(1, min(args.max_new_tokens, cfg.max_supported_new_tokens)),
        temperature=args.temperature,
        top_k=args.top_k,
        device=args.device,
    )

    print(f"checkpoint: {ckpt}")
    print(f"prompt: {args.prompt}")
    print(f"generated_tokens: {max(1, min(args.max_new_tokens, cfg.max_supported_new_tokens))}")
    print(f"avg_ms_per_token: {(elapsed * 1000.0) / max(1, args.max_new_tokens):.3f}")
    print(f"fallback: {'yes' if fallback_used else 'no'}")
    if fallback_used:
        print("[fallback used]")
    print("generated text:")
    print(reply)
    print("\nraw model text:")
    print(safe_decode(raw))
    if "train_loss" in obj:
        print(f"\ncheckpoint train_loss: {obj.get('train_loss'):.4f}")
        print(f"checkpoint val_loss: {obj.get('val_loss'):.4f}")


if __name__ == "__main__":
    main()
