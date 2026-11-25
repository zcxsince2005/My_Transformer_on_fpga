#include "gpt_accel.h"
#include <iostream>

static gpt_i8 a[GPT_BLOCK_SIZE * GPT_N_EMBD];
static gpt_i8 b[3 * GPT_N_EMBD * GPT_N_EMBD];
static gpt_i16 c[GPT_MAX_SUPPORTED_NEW_TOKENS * GPT_N_EMBD * 2];
static gpt_i32 out[GPT_BLOCK_SIZE * 3 * GPT_N_EMBD];

static int gelu_ref(int x) {
    if (x < -2048) return 0;
    if (x > 2048) return x;
    return (x * (x + 2048)) >> 12;
}

static int run_matmul_test() {
    for (int i = 0; i < 32; i++) {
        a[i] = (gpt_i8)((i % 7) - 3);
        b[i] = (gpt_i8)((i % 5) - 2);
    }
    for (int i = 0; i < 16; i++) out[i] = 0;

    gpt_accel(a, b, c, out, 4, 4, 8, GPT_MODE_MATMUL);

    int errors = 0;
    for (int r = 0; r < 4; r++) {
        for (int col = 0; col < 4; col++) {
            int ref = 0;
            for (int k = 0; k < 8; k++) {
                ref += (int)a[r * 8 + k] * (int)b[k * 4 + col];
            }
            if ((int)out[r * 4 + col] != ref) {
                errors++;
            }
        }
    }
    return errors;
}

static int run_qkv_test() {
    const int tokens = 2;
    const int embd = 8;
    const int qkv_cols = 3 * embd;

    for (int i = 0; i < tokens * embd; i++) {
        a[i] = (gpt_i8)((i % 9) - 4);
    }
    for (int i = 0; i < qkv_cols * embd; i++) {
        b[i] = (gpt_i8)((i % 11) - 5);
    }
    for (int i = 0; i < tokens * qkv_cols; i++) out[i] = 0;

    gpt_accel(a, b, c, out, tokens, qkv_cols, embd, GPT_MODE_QKV);

    int errors = 0;
    for (int t = 0; t < tokens; t++) {
        for (int o = 0; o < qkv_cols; o++) {
            int ref = 0;
            for (int k = 0; k < embd; k++) {
                ref += (int)a[t * embd + k] * (int)b[o * embd + k];
            }
            int got = (int)out[t * qkv_cols + o];
            if (got != ref) {
                if (errors < 8) {
                    std::cout << "QKV mismatch t=" << t << " o=" << o
                              << " got=" << got << " ref=" << ref << std::endl;
                }
                errors++;
            }
        }
    }
    return errors;
}

static int run_mlp_test() {
    const int tokens = 1;
    const int embd = 8;
    const int hidden = 16;
    int hidden_ref[hidden];

    for (int i = 0; i < tokens * embd; i++) {
        a[i] = (gpt_i8)((i % 7) - 3);
    }
    for (int i = 0; i < hidden * embd; i++) {
        b[i] = (gpt_i8)((i % 9) - 4);
    }
    for (int i = 0; i < embd * hidden; i++) {
        c[i] = (gpt_i16)((i % 13) - 6);
    }
    for (int i = 0; i < tokens * embd; i++) out[i] = 0;

    gpt_accel(a, b, c, out, tokens, embd, hidden, GPT_MODE_MLP);

    for (int h = 0; h < hidden; h++) {
        int acc = 0;
        for (int k = 0; k < embd; k++) {
            acc += (int)a[k] * (int)b[h * embd + k];
        }
        hidden_ref[h] = gelu_ref(acc);
    }

    int errors = 0;
    for (int o = 0; o < embd; o++) {
        int ref = 0;
        for (int h = 0; h < hidden; h++) {
            ref += (hidden_ref[h] >> 8) * (int)c[o * hidden + h];
        }
        int got = (int)out[o];
        if (got != ref) {
            if (errors < 8) {
                std::cout << "MLP mismatch o=" << o
                          << " got=" << got << " ref=" << ref << std::endl;
            }
            errors++;
        }
    }
    return errors;
}

static int run_layernorm_test() {
    const int tokens = 2;
    const int embd = 8;

    for (int i = 0; i < tokens * embd; i++) {
        a[i] = (gpt_i8)((i % 15) - 7);
    }
    for (int i = 0; i < tokens * embd; i++) out[i] = 0;

    gpt_accel(a, b, c, out, tokens, embd, 0, GPT_MODE_LAYERNORM);

    int errors = 0;
    for (int t = 0; t < tokens; t++) {
        int sum = 0;
        for (int i = 0; i < embd; i++) {
            sum += (int)a[t * embd + i];
        }
        int mean = sum / embd;
        for (int i = 0; i < embd; i++) {
            int ref = ((int)a[t * embd + i] - mean) << 8;
            int got = (int)out[t * embd + i];
            if (got != ref) {
                if (errors < 8) {
                    std::cout << "LayerNorm mismatch t=" << t << " i=" << i
                              << " got=" << got << " ref=" << ref << std::endl;
                }
                errors++;
            }
        }
    }
    return errors;
}

int main() {
    int errors = 0;
    int e = 0;

    e = run_matmul_test();
    std::cout << "matmul errors=" << e << std::endl;
    errors += e;

    e = run_qkv_test();
    std::cout << "qkv errors=" << e << std::endl;
    errors += e;

    e = run_mlp_test();
    std::cout << "mlp errors=" << e << std::endl;
    errors += e;

    e = run_layernorm_test();
    std::cout << "layernorm errors=" << e << std::endl;
    errors += e;

    if (errors) {
        std::cout << "FAILED errors=" << errors << std::endl;
        return 1;
    }
    std::cout << "PASSED" << std::endl;
    return 0;
}
