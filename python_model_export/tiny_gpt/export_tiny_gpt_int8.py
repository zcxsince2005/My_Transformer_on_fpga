from __future__ import annotations

import argparse
import re
from pathlib import Path

import numpy as np
import torch

from tiny_gpt_config import DEFAULT_CONFIG, TinyGPTConfig
from tokenizer import AsciiTokenizer, build_c_vocab_header
from train_tiny_gpt import TinyGPT


def c_name(name: str) -> str:
    return re.sub(r"[^0-9a-zA-Z_]", "_", name)


def quantize(arr: np.ndarray) -> tuple[np.ndarray, float]:
    arr = arr.astype(np.float32)
    max_abs = float(np.max(np.abs(arr))) if arr.size else 0.0
    scale = max(max_abs / 127.0, 1.0 / 32768.0)
    q = np.clip(np.round(arr / scale), -127, 127).astype(np.int8)
    return q, scale


def emit_int8_array(name: str, arr: np.ndarray) -> list[str]:
    flat = arr.flatten()
    lines = [f"static const int8_t {name}[{flat.size}] = {{"]
    for i in range(0, flat.size, 20):
        lines.append("    " + ", ".join(str(int(x)) for x in flat[i : i + 20]) + ",")
    lines.append("};")
    return lines


def emit_i32_array(name: str, arr: np.ndarray) -> list[str]:
    flat = arr.astype(np.int32).flatten()
    lines = [f"static const int32_t {name}[{flat.size}] = {{"]
    for i in range(0, flat.size, 12):
        lines.append("    " + ", ".join(str(int(x)) for x in flat[i : i + 12]) + ",")
    lines.append("};")
    return lines


def write_config_header(out_dir: Path, cfg: TinyGPTConfig) -> None:
    text = f"""#ifndef GPT_CONFIG_H
#define GPT_CONFIG_H

#define GPT_VOCAB_SIZE {cfg.vocab_size}
#define GPT_BLOCK_SIZE {cfg.block_size}
#define GPT_N_EMBD {cfg.n_embd}
#define GPT_N_HEAD {cfg.n_head}
#define GPT_N_LAYER {cfg.n_layer}
#define GPT_HEAD_SIZE {cfg.head_size}
#define GPT_MLP_HIDDEN {cfg.mlp_hidden}
#define GPT_MAX_NEW_TOKENS {cfg.max_new_tokens}
#define GPT_MIN_NEW_TOKENS {cfg.min_new_tokens}
#define GPT_MAX_SUPPORTED_NEW_TOKENS {cfg.max_supported_new_tokens}

#endif
"""
    (out_dir / "gpt_config.h").write_text(text, encoding="utf-8")


def write_weights_header(out_dir: Path, model: TinyGPT) -> None:
    lines: list[str] = [
        "#ifndef GPT_WEIGHTS_H",
        "#define GPT_WEIGHTS_H",
        "",
        "#include <stdint.h>",
        '#include "gpt_config.h"',
        "",
        "typedef struct {",
        "    const char *name;",
        "    int rows;",
        "    int cols;",
        "    float scale;",
        "    const int8_t *data;",
        "} GptInt8Tensor;",
        "",
    ]
    table = []
    with torch.no_grad():
        state = model.state_dict()
        for key in sorted(state.keys()):
            if key.endswith("bias") or key.endswith("weight"):
                arr = state[key].detach().cpu().numpy()
                if arr.dtype.kind not in ("f", "i"):
                    continue
                q, scale = quantize(arr)
                name = "GPTW_" + c_name(key)
                lines.extend(emit_int8_array(name, q))
                lines.append("")
                rows = int(arr.shape[0]) if arr.ndim >= 1 else 1
                cols = int(np.prod(arr.shape[1:])) if arr.ndim >= 2 else 1
                table.append((key, rows, cols, scale, name))
    lines.append(f"#define GPT_WEIGHT_TENSOR_COUNT {len(table)}")
    lines.append("static const GptInt8Tensor GPT_WEIGHT_TENSORS[GPT_WEIGHT_TENSOR_COUNT] = {")
    for key, rows, cols, scale, name in table:
        lines.append(f'    {{"{key}", {rows}, {cols}, {scale:.10e}f, {name}}},')
    lines.extend(["};", "", "#endif", ""])
    (out_dir / "gpt_weights.h").write_text("\n".join(lines), encoding="utf-8")


def write_test_vectors(out_dir: Path, model: TinyGPT, prompt: str, max_new_tokens: int) -> None:
    tokenizer = AsciiTokenizer(model.cfg.vocab_size)
    ids = tokenizer.encode(prompt)
    x = torch.tensor([ids[-model.cfg.block_size :]], dtype=torch.long)
    model.eval()
    with torch.no_grad():
        y = model.generate(x, max_new_tokens=max_new_tokens, temperature=0.8)[0].cpu().numpy()
    lines = [
        "#ifndef GPT_TEST_VECTORS_H",
        "#define GPT_TEST_VECTORS_H",
        "",
        '#include "gpt_config.h"',
        "",
        f"#define GPT_TEST_PROMPT_LEN {len(ids)}",
        f"#define GPT_TEST_OUTPUT_LEN {len(y)}",
    ]
    lines.extend(emit_i32_array("GPT_TEST_PROMPT", np.array(ids, dtype=np.int32)))
    lines.append("")
    lines.extend(emit_i32_array("GPT_TEST_OUTPUT", y.astype(np.int32)))
    lines.extend(["", "#endif", ""])
    (out_dir / "gpt_test_vectors.h").write_text("\n".join(lines), encoding="utf-8")


def load_model(checkpoint: Path, device: str) -> TinyGPT:
    cfg = DEFAULT_CONFIG
    model = TinyGPT(cfg).to(device)
    if checkpoint.exists():
        obj = torch.load(checkpoint, map_location=device)
        model.load_state_dict(obj["model"])
        print(f"loaded checkpoint: {checkpoint}")
    else:
        print(f"checkpoint not found, exporting deterministic initial weights: {checkpoint}")
    return model.cpu()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--checkpoint", default="checkpoints/tiny_gpt.pt")
    parser.add_argument("--out-dir", default="exported")
    parser.add_argument("--prompt", default="User: hello\nAI:")
    parser.add_argument("--max-new-tokens", type=int, default=16)
    args = parser.parse_args()

    here = Path(__file__).resolve().parent
    out_dir = (here / args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    model = load_model((here / args.checkpoint).resolve(), "cpu")

    write_config_header(out_dir, model.cfg)
    (out_dir / "gpt_vocab.h").write_text(build_c_vocab_header(model.cfg.vocab_size), encoding="utf-8")
    write_weights_header(out_dir, model)
    write_test_vectors(out_dir, model, args.prompt, args.max_new_tokens)
    print(f"exported INT8 headers to: {out_dir}")


if __name__ == "__main__":
    main()
