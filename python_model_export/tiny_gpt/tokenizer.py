from __future__ import annotations


class AsciiTokenizer:
    """Fixed 7-bit ASCII character tokenizer for baremetal-friendly GPT demos."""

    def __init__(self, vocab_size: int = 128):
        if vocab_size not in (128, 256):
            raise ValueError("vocab_size must be 128 or 256")
        self.vocab_size = vocab_size
        self.unk_id = ord("?")

    def encode(self, text: str) -> list[int]:
        ids: list[int] = []
        for ch in text:
            code = ord(ch)
            if 0 <= code < self.vocab_size:
                ids.append(code)
            else:
                ids.append(self.unk_id)
        return ids

    def decode(self, ids: list[int]) -> str:
        out = []
        for idx in ids:
            if idx == 0:
                continue
            if 0 <= idx < self.vocab_size:
                out.append(chr(idx))
            else:
                out.append("?")
        return "".join(out)


def build_c_vocab_header(vocab_size: int = 128) -> str:
    lines = [
        "#ifndef GPT_VOCAB_H",
        "#define GPT_VOCAB_H",
        "",
        "#include <stdint.h>",
        "",
        f"#define GPT_VOCAB_SIZE {vocab_size}",
        "#define GPT_UNK_TOKEN 63",
        "",
        "static const char GPT_ID_TO_CHAR[GPT_VOCAB_SIZE] = {",
    ]
    vals = []
    for i in range(vocab_size):
        if 32 <= i <= 126:
            vals.append("'" + chr(i).replace("\\", "\\\\").replace("'", "\\'") + "'")
        elif i == 10:
            vals.append("'\\n'")
        elif i == 13:
            vals.append("'\\r'")
        elif i == 9:
            vals.append("'\\t'")
        else:
            vals.append("' '")
    for i in range(0, len(vals), 16):
        lines.append("    " + ", ".join(vals[i:i + 16]) + ",")
    lines.extend(["};", "", "#endif"])
    return "\n".join(lines) + "\n"
