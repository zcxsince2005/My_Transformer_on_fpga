from __future__ import annotations

import argparse
import json
import random
import re
import textwrap
import urllib.request
from pathlib import Path

from tiny_gpt_config import DEFAULT_CONFIG
from tokenizer import build_c_vocab_header


ALPACA_URL = "https://raw.githubusercontent.com/tatsu-lab/stanford_alpaca/main/alpaca_data.json"
DOLLY_URL = "https://huggingface.co/datasets/databricks/databricks-dolly-15k/resolve/main/databricks-dolly-15k.jsonl"

SAFE_CHARS = set(
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789"
    " .,!?;:'\"-_/()\n"
)

CORE_QA = {
    "hello": [
        "Hello. I am a tiny assistant model for an FPGA demo.",
        "Hi. I can answer short questions about AI, FPGA, Zynq, HLS, and simple technology topics.",
        "Hello. Ask me a short question and I will answer in clear English.",
    ],
    "who are you": [
        "I am a tiny GPT style assistant trained for a Zynq MPSoC FPGA demonstration.",
        "I am a small character level language model that can answer simple questions.",
        "I am a compact assistant model. I run as a Python demo first and can later be exported.",
    ],
    "what is fpga": [
        "An FPGA is a programmable chip. It can be configured to implement custom hardware circuits.",
        "An FPGA is reconfigurable logic used for parallel hardware acceleration and low latency processing.",
        "An FPGA can build custom datapaths for tasks such as matrix multiply, signal processing, and control.",
    ],
    "what is zynq": [
        "Zynq combines ARM processors and FPGA programmable logic on one chip.",
        "A Zynq MPSoC has a processor system for software and programmable logic for hardware acceleration.",
        "Zynq is useful because software can control custom hardware accelerators through AXI interfaces.",
    ],
    "what is hls": [
        "HLS converts C or C++ code into hardware logic for an FPGA.",
        "High level synthesis helps turn loops, arrays, and functions into pipelined hardware blocks.",
        "HLS pragmas such as pipeline and unroll guide the tool to make faster circuits.",
    ],
    "tell me about ai": [
        "AI uses data and models to make predictions, classify inputs, or generate text.",
        "AI systems learn patterns from examples. Better data usually improves the model behavior.",
        "A small AI demo can show classification, text generation, and hardware acceleration.",
    ],
    "what is transformer": [
        "A transformer is a neural network architecture based on attention and feed forward layers.",
        "Transformers process tokens and predict the next token from the context.",
        "A tiny transformer can generate short text, but it is much smaller than a real large language model.",
    ],
    "what is lenet": [
        "LeNet is a small convolutional neural network often used for handwritten digit recognition.",
        "The LeNet demo sends an image to an accelerator and reads class scores for digits zero to nine.",
        "LeNet is useful for a small FPGA demo because it is simple and fast.",
    ],
}

GENERAL_QA = {
    "what is a computer": "A computer is a machine that follows instructions to process data and run programs.",
    "what is software": "Software is a set of instructions that tells hardware what to do.",
    "what is hardware": "Hardware is the physical part of a computer system, such as chips, memory, and boards.",
    "what is memory": "Memory stores data and instructions while a program is running.",
    "what is python": "Python is a programming language often used for scripts, data processing, and machine learning.",
    "what is c language": "C is a programming language often used for systems, embedded software, and hardware tools.",
    "what is uart": "UART is a serial communication interface that sends and receives bytes over a simple connection.",
    "what is axi": "AXI is an interface protocol used to connect processors, memory, and IP blocks in many FPGA systems.",
    "what is ddr": "DDR is external memory used to store large data buffers and program data.",
    "what is a token": "A token is a unit of text that a language model reads or generates.",
    "what is quantization": "Quantization stores numbers with fewer bits, such as INT8, to reduce memory and computation cost.",
    "why use gpu": "A GPU can train neural networks faster because it runs many math operations in parallel.",
    "why use fpga": "An FPGA can provide custom parallel hardware with predictable latency and efficient data movement.",
}


def download(url: str, out: Path, timeout: int = 120) -> str:
    if out.exists() and out.stat().st_size > 1024:
        return out.read_text(encoding="utf-8", errors="ignore")
    print(f"downloading: {url}")
    req = urllib.request.Request(url, headers={"User-Agent": "tiny-gpt-data-prep"})
    with urllib.request.urlopen(req, timeout=timeout) as response:
        data = response.read().decode("utf-8", errors="ignore")
    out.write_text(data, encoding="utf-8")
    return data


def clean_text(text: str) -> str:
    text = text.replace("\r\n", "\n").replace("\r", "\n").replace("\t", " ")
    text = "".join(ch if ch in SAFE_CHARS else " " for ch in text)
    text = re.sub(r"[ ]+", " ", text)
    text = re.sub(r" *\n *", "\n", text)
    text = re.sub(r"\n{4,}", "\n\n\n", text)
    return text.strip()


def format_pair(prompt: str, answer: str, style: str) -> str:
    prompt = clean_text(prompt)
    answer = clean_text(answer)
    if not prompt or not answer:
        return ""
    if style == "assistant":
        return f"User: {prompt}\nAssistant: {answer}\n"
    if style == "qa":
        return f"Question: {prompt}\nAnswer: {answer}\n"
    if style == "human":
        return f"Human: {prompt}\nAI: {answer}\n"
    return f"Instruction: {prompt}\nResponse: {answer}\n"


def load_alpaca(data_dir: Path, max_records: int) -> list[str]:
    records: list[str] = []
    try:
        raw = download(ALPACA_URL, data_dir / "alpaca_data.json")
        items = json.loads(raw)
        for item in items[:max_records]:
            inst = str(item.get("instruction", "")).strip()
            inp = str(item.get("input", "")).strip()
            out = str(item.get("output", "")).strip()
            prompt = inst if not inp else f"{inst}\n{inp}"
            text = format_pair(prompt, out, random.choice(["assistant", "qa", "human", "instruction"]))
            if text:
                records.append(text)
    except Exception as exc:
        print(f"warning: Alpaca download/parse failed: {exc}")
    return records


def load_dolly(data_dir: Path, max_records: int) -> list[str]:
    records: list[str] = []
    try:
        raw = download(DOLLY_URL, data_dir / "databricks_dolly_15k.jsonl")
        for line in raw.splitlines()[:max_records]:
            if not line.strip():
                continue
            item = json.loads(line)
            inst = str(item.get("instruction", "")).strip()
            ctx = str(item.get("context", "")).strip()
            resp = str(item.get("response", "")).strip()
            prompt = inst if not ctx else f"{inst}\n{ctx}"
            text = format_pair(prompt, resp, random.choice(["assistant", "qa", "human", "instruction"]))
            if text:
                records.append(text)
    except Exception as exc:
        print(f"warning: Dolly download/parse failed: {exc}")
    return records


def synthetic_records(count: int) -> list[str]:
    rng = random.Random(23)
    prompts = list(CORE_QA.keys()) + list(GENERAL_QA.keys())
    records: list[str] = []
    for _ in range(count):
        prompt = rng.choice(prompts)
        if prompt in CORE_QA:
            answer = rng.choice(CORE_QA[prompt])
        else:
            answer = GENERAL_QA[prompt]
        prefix = rng.choice(["", "please ", "can you explain ", "tell me "])
        if prefix and not prompt.startswith(("tell", "can")):
            prompt_text = prefix + prompt
        else:
            prompt_text = prompt
        style = rng.choice(["assistant", "assistant", "qa", "human", "instruction"])
        records.append(format_pair(prompt_text, answer, style))

    short_lessons = [
        "A helpful assistant gives short, clear answers. It avoids random symbols and explains one idea at a time.",
        "When the question is about FPGA, mention programmable logic, custom hardware, and acceleration.",
        "When the question is about Zynq, mention ARM processors, programmable logic, and AXI control.",
        "When the question is about AI, mention data, models, learning, prediction, and generation.",
        "When the question is about HLS, mention C code, hardware logic, pipeline, and unroll.",
    ]
    for lesson in short_lessons * max(1, count // 500):
        records.append(f"System: {lesson}\n")
    return records


def split_records(records: list[str], val_fraction: float) -> tuple[str, str]:
    rng = random.Random(37)
    rng.shuffle(records)
    val_count = max(1, int(len(records) * val_fraction))
    val = "\n".join(records[:val_count])
    train = "\n".join(records[val_count:])
    return train, val


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--max-alpaca", type=int, default=16000)
    parser.add_argument("--max-dolly", type=int, default=12000)
    parser.add_argument("--synthetic", type=int, default=40000)
    parser.add_argument("--val-fraction", type=float, default=0.04)
    args = parser.parse_args()

    random.seed(5)
    here = Path(__file__).resolve().parent
    data_dir = here / "data"
    exported_dir = here / "exported"
    data_dir.mkdir(parents=True, exist_ok=True)
    exported_dir.mkdir(parents=True, exist_ok=True)

    records: list[str] = []
    alpaca = load_alpaca(data_dir, args.max_alpaca)
    dolly = load_dolly(data_dir, args.max_dolly)
    synth = synthetic_records(args.synthetic)
    records.extend(alpaca)
    records.extend(dolly)
    records.extend(synth)
    if not records:
        raise RuntimeError("no dataset records available")

    raw = clean_text("\n".join(records)) + "\n"
    train, val = split_records([r for r in raw.split("\n\n") if r.strip()], args.val_fraction)
    train = clean_text(train) + "\n"
    val = clean_text(val) + "\n"

    (data_dir / "raw_dataset.txt").write_text(raw, encoding="utf-8")
    (data_dir / "train.txt").write_text(train, encoding="utf-8")
    (data_dir / "val.txt").write_text(val, encoding="utf-8")
    (exported_dir / "gpt_vocab.h").write_text(build_c_vocab_header(DEFAULT_CONFIG.vocab_size), encoding="utf-8")

    info = textwrap.dedent(
        f"""
        Dataset preparation for tiny_gpt assistant training
        sources:
          Stanford Alpaca data: {ALPACA_URL}
          Databricks Dolly 15k: {DOLLY_URL}
          synthetic assistant QA: generated locally for FPGA AI Zynq HLS and common technical questions
        source_notes:
          Alpaca is an instruction-following research dataset from Stanford Alpaca.
          Dolly 15k is an open instruction dataset from Databricks on Hugging Face.
          Synthetic records are generated by prepare_dataset.py and are project-local.
        downloaded_records:
          alpaca: {len(alpaca)}
          dolly: {len(dolly)}
          synthetic: {len(synth)}
        total_records: {len(records)}
        raw_chars: {len(raw)}
        train_chars: {len(train)}
        val_chars: {len(val)}
        vocab_size: {DEFAULT_CONFIG.vocab_size}
        style: modern assistant dialogue and instruction response text
        active_training_files:
          train.txt
          val.txt
        cache_or_reference_files:
          alpaca_data.json
          databricks_dolly_15k.jsonl
          raw_dataset.txt
          dataset_info.txt
        """
    ).strip()
    (data_dir / "dataset_info.txt").write_text(info + "\n", encoding="utf-8")

    print(info)
    print("\npreview:")
    print(raw[:1200])


if __name__ == "__main__":
    main()
