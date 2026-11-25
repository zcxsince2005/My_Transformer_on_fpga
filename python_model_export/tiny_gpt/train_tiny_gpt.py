from __future__ import annotations

import argparse
import math
import time
from pathlib import Path

import torch
import torch.nn as nn
from torch.nn import functional as F

from tiny_gpt_config import DEFAULT_CONFIG, TinyGPTConfig
from tokenizer import AsciiTokenizer


class CausalSelfAttention(nn.Module):
    def __init__(self, cfg: TinyGPTConfig):
        super().__init__()
        self.cfg = cfg
        self.c_attn = nn.Linear(cfg.n_embd, 3 * cfg.n_embd)
        self.c_proj = nn.Linear(cfg.n_embd, cfg.n_embd)
        self.attn_dropout = nn.Dropout(cfg.dropout)
        self.resid_dropout = nn.Dropout(cfg.dropout)
        self.register_buffer(
            "bias",
            torch.tril(torch.ones(cfg.block_size, cfg.block_size)).view(1, 1, cfg.block_size, cfg.block_size),
        )

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        b, t, c = x.size()
        qkv = self.c_attn(x)
        q, k, v = qkv.split(self.cfg.n_embd, dim=2)
        q = q.view(b, t, self.cfg.n_head, self.cfg.head_size).transpose(1, 2)
        k = k.view(b, t, self.cfg.n_head, self.cfg.head_size).transpose(1, 2)
        v = v.view(b, t, self.cfg.n_head, self.cfg.head_size).transpose(1, 2)
        att = (q @ k.transpose(-2, -1)) * (1.0 / math.sqrt(k.size(-1)))
        att = att.masked_fill(self.bias[:, :, :t, :t] == 0, float("-inf"))
        att = F.softmax(att, dim=-1)
        att = self.attn_dropout(att)
        y = att @ v
        y = y.transpose(1, 2).contiguous().view(b, t, c)
        return self.resid_dropout(self.c_proj(y))


class MLP(nn.Module):
    def __init__(self, cfg: TinyGPTConfig):
        super().__init__()
        self.c_fc = nn.Linear(cfg.n_embd, cfg.mlp_hidden)
        self.c_proj = nn.Linear(cfg.mlp_hidden, cfg.n_embd)
        self.dropout = nn.Dropout(cfg.dropout)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return self.dropout(self.c_proj(F.gelu(self.c_fc(x))))


class Block(nn.Module):
    def __init__(self, cfg: TinyGPTConfig):
        super().__init__()
        self.ln_1 = nn.LayerNorm(cfg.n_embd)
        self.attn = CausalSelfAttention(cfg)
        self.ln_2 = nn.LayerNorm(cfg.n_embd)
        self.mlp = MLP(cfg)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        x = x + self.attn(self.ln_1(x))
        x = x + self.mlp(self.ln_2(x))
        return x


class TinyGPT(nn.Module):
    def __init__(self, cfg: TinyGPTConfig = DEFAULT_CONFIG):
        super().__init__()
        self.cfg = cfg
        self.transformer = nn.ModuleDict(
            dict(
                wte=nn.Embedding(cfg.vocab_size, cfg.n_embd),
                wpe=nn.Embedding(cfg.block_size, cfg.n_embd),
                drop=nn.Dropout(cfg.dropout),
                h=nn.ModuleList([Block(cfg) for _ in range(cfg.n_layer)]),
                ln_f=nn.LayerNorm(cfg.n_embd),
            )
        )
        self.lm_head = nn.Linear(cfg.n_embd, cfg.vocab_size, bias=False)
        self.apply(self._init_weights)

    def _init_weights(self, module: nn.Module) -> None:
        if isinstance(module, nn.Linear):
            nn.init.normal_(module.weight, mean=0.0, std=0.02)
            if module.bias is not None:
                nn.init.zeros_(module.bias)
        elif isinstance(module, nn.Embedding):
            nn.init.normal_(module.weight, mean=0.0, std=0.02)

    def forward(self, idx: torch.Tensor, targets: torch.Tensor | None = None):
        b, t = idx.size()
        if t > self.cfg.block_size:
            raise ValueError("sequence length exceeds block_size")
        pos = torch.arange(0, t, dtype=torch.long, device=idx.device)
        x = self.transformer.drop(self.transformer.wte(idx) + self.transformer.wpe(pos))
        for block in self.transformer.h:
            x = block(x)
        x = self.transformer.ln_f(x)
        logits = self.lm_head(x)
        loss = None
        if targets is not None:
            loss = F.cross_entropy(logits.view(-1, logits.size(-1)), targets.view(-1))
        return logits, loss

    @torch.no_grad()
    def generate(
        self,
        idx: torch.Tensor,
        max_new_tokens: int = 128,
        temperature: float = 0.9,
        top_k: int | None = None,
    ) -> torch.Tensor:
        for _ in range(max_new_tokens):
            idx_cond = idx[:, -self.cfg.block_size :]
            logits, _ = self(idx_cond)
            logits = logits[:, -1, :] / max(temperature, 1e-4)
            if top_k is not None and top_k > 0:
                v, _ = torch.topk(logits, min(top_k, logits.size(-1)))
                logits[logits < v[:, [-1]]] = -float("inf")
            probs = F.softmax(logits, dim=-1)
            idx_next = torch.multinomial(probs, num_samples=1)
            idx = torch.cat((idx, idx_next), dim=1)
        return idx


def load_corpus(path: Path, tokenizer: AsciiTokenizer) -> torch.Tensor:
    text = path.read_text(encoding="utf-8")
    ids = tokenizer.encode(text)
    return torch.tensor(ids, dtype=torch.long)


def get_batch(data: torch.Tensor, block_size: int, batch_size: int, device: str):
    if len(data) <= block_size + 1:
        data = data.repeat((block_size + 2) // max(len(data), 1) + 1)
    ix = torch.randint(len(data) - block_size - 1, (batch_size,))
    x = torch.stack([data[i : i + block_size] for i in ix]).to(device)
    y = torch.stack([data[i + 1 : i + block_size + 1] for i in ix]).to(device)
    return x, y


@torch.no_grad()
def estimate_loss(model: TinyGPT, train_data: torch.Tensor, val_data: torch.Tensor, cfg: TinyGPTConfig, device: str, eval_iters: int):
    out = {}
    was_training = model.training
    model.eval()
    for split, data in (("train", train_data), ("val", val_data)):
        losses = torch.zeros(eval_iters)
        for k in range(eval_iters):
            xb, yb = get_batch(data, cfg.block_size, cfg.batch_size, device)
            _, loss = model(xb, yb)
            losses[k] = loss.item()
        out[split] = float(losses.mean())
    if was_training:
        model.train()
    return out


def count_parameters(model: nn.Module) -> int:
    return sum(p.numel() for p in model.parameters())


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--train", default="data/train.txt")
    parser.add_argument("--val", default="data/val.txt")
    parser.add_argument("--out", default="checkpoints/tiny_gpt.pt")
    parser.add_argument("--max-iters", type=int, default=DEFAULT_CONFIG.max_iters)
    parser.add_argument("--batch-size", type=int, default=DEFAULT_CONFIG.batch_size)
    parser.add_argument("--lr", type=float, default=DEFAULT_CONFIG.learning_rate)
    parser.add_argument("--eval-interval", type=int, default=DEFAULT_CONFIG.eval_interval)
    parser.add_argument("--eval-iters", type=int, default=DEFAULT_CONFIG.eval_iters)
    parser.add_argument("--device", default="cuda" if torch.cuda.is_available() else "cpu")
    args = parser.parse_args()

    here = Path(__file__).resolve().parent
    train_path = (here / args.train).resolve()
    val_path = (here / args.val).resolve()
    out_path = (here / args.out).resolve()
    out_path.parent.mkdir(parents=True, exist_ok=True)

    cfg = DEFAULT_CONFIG
    cfg.batch_size = args.batch_size
    cfg.learning_rate = args.lr
    cfg.max_iters = args.max_iters
    cfg.eval_interval = args.eval_interval
    cfg.eval_iters = args.eval_iters
    tokenizer = AsciiTokenizer(cfg.vocab_size)
    train_data = load_corpus(train_path, tokenizer)
    val_data = load_corpus(val_path, tokenizer)
    model = TinyGPT(cfg).to(args.device)
    optimizer = torch.optim.AdamW(model.parameters(), lr=args.lr)
    print(f"device: {args.device}")
    print(f"parameters: {count_parameters(model)}")
    print(f"train chars: {len(train_data)}")
    print(f"val chars: {len(val_data)}")

    t0 = time.perf_counter()
    last_losses = {"train": float("nan"), "val": float("nan")}
    model.train()
    for step in range(args.max_iters):
        if step % args.eval_interval == 0 or step == args.max_iters - 1:
            last_losses = estimate_loss(model, train_data, val_data, cfg, args.device, args.eval_iters)
            print(f"step {step:04d} train {last_losses['train']:.4f} val {last_losses['val']:.4f}")
        xb, yb = get_batch(train_data, cfg.block_size, args.batch_size, args.device)
        _, loss = model(xb, yb)
        optimizer.zero_grad(set_to_none=True)
        loss.backward()
        torch.nn.utils.clip_grad_norm_(model.parameters(), 1.0)
        optimizer.step()

    elapsed = time.perf_counter() - t0
    torch.save(
        {
            "config": cfg.__dict__,
            "model": model.state_dict(),
            "train_loss": last_losses["train"],
            "val_loss": last_losses["val"],
            "max_iters": args.max_iters,
            "parameters": count_parameters(model),
            "elapsed_sec": elapsed,
        },
        out_path,
    )
    print(f"final train loss: {last_losses['train']:.4f}")
    print(f"final val loss: {last_losses['val']:.4f}")
    print(f"elapsed_sec: {elapsed:.1f}")
    print(f"saved checkpoint: {out_path}")


if __name__ == "__main__":
    main()
