from dataclasses import dataclass


@dataclass
class TinyGPTConfig:
    vocab_size: int = 128
    block_size: int = 128
    n_embd: int = 96
    n_head: int = 4
    n_layer: int = 2
    dropout: float = 0.1
    max_new_tokens: int = 128
    min_new_tokens: int = 64
    max_supported_new_tokens: int = 200
    batch_size: int = 32
    learning_rate: float = 3e-4
    max_iters: int = 2000
    eval_interval: int = 200
    eval_iters: int = 50

    @property
    def head_size(self) -> int:
        return self.n_embd // self.n_head

    @property
    def mlp_hidden(self) -> int:
        return 4 * self.n_embd


DEFAULT_CONFIG = TinyGPTConfig()
