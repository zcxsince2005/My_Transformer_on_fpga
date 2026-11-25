#include "gpt_accel.h"

static gpt_i32 gelu_approx_i32(gpt_i32 x) {
#pragma HLS INLINE
    // Piecewise fixed-point GELU-like activation for INT16 intermediate values.
    if (x < -2048) return 0;
    if (x > 2048) return x;
    return (x * (x + 2048)) >> 12;
}

static gpt_i16 exp_lut_approx(gpt_i32 x) {
#pragma HLS INLINE
    if (x < -4096) return 0;
    if (x > 0) x = 0;
    // 1.0, 0.78, 0.61, 0.47, 0.37, 0.29, 0.22, 0.17 in Q12.
    static const gpt_i16 lut[8] = {4096, 3195, 2490, 1938, 1506, 1171, 910, 707};
#pragma HLS ARRAY_PARTITION variable=lut complete
    int idx = (int)((-x) >> 9);
    if (idx > 7) idx = 7;
    return lut[idx];
}

static void matmul_int8_kernel(
    const gpt_i8 *a,
    const gpt_i8 *b,
    gpt_i32 *out,
    int rows,
    int cols,
    int inner
) {
    static gpt_i8 a_tile[GPT_LINEAR_PARALLEL_FACTOR];
    static gpt_i8 b_tile[GPT_LINEAR_PARALLEL_FACTOR][GPT_MATMUL_OUT_PARALLEL_FACTOR];
#pragma HLS ARRAY_PARTITION variable=a_tile complete
#pragma HLS ARRAY_PARTITION variable=b_tile complete dim=0
row_loop:
    for (int r = 0; r < rows; r++) {
    col_loop:
        for (int c = 0; c < cols; c += GPT_MATMUL_OUT_PARALLEL_FACTOR) {
#pragma HLS PIPELINE II=1
            gpt_i32 acc[GPT_MATMUL_OUT_PARALLEL_FACTOR];
#pragma HLS ARRAY_PARTITION variable=acc complete
        init_acc:
            for (int cc = 0; cc < GPT_MATMUL_OUT_PARALLEL_FACTOR; cc++) {
#pragma HLS UNROLL
                acc[cc] = 0;
            }
        inner_loop:
            for (int k = 0; k < inner; k += GPT_LINEAR_PARALLEL_FACTOR) {
                for (int u = 0; u < GPT_LINEAR_PARALLEL_FACTOR; u++) {
#pragma HLS UNROLL
                    int kk = k + u;
                    a_tile[u] = (kk < inner) ? a[r * inner + kk] : (gpt_i8)0;
                load_b_cols:
                    for (int cc = 0; cc < GPT_MATMUL_OUT_PARALLEL_FACTOR; cc++) {
#pragma HLS UNROLL
                        int col = c + cc;
                        b_tile[u][cc] = (kk < inner && col < cols) ? b[kk * cols + col] : (gpt_i8)0;
                    }
                }
                for (int u = 0; u < GPT_LINEAR_PARALLEL_FACTOR; u++) {
#pragma HLS UNROLL
                mac_cols:
                    for (int cc = 0; cc < GPT_MATMUL_OUT_PARALLEL_FACTOR; cc++) {
#pragma HLS UNROLL
                        acc[cc] += (gpt_i16)a_tile[u] * (gpt_i16)b_tile[u][cc];
                    }
                }
            }
        store_cols:
            for (int cc = 0; cc < GPT_MATMUL_OUT_PARALLEL_FACTOR; cc++) {
#pragma HLS UNROLL
                int col = c + cc;
                if (col < cols) {
                    out[r * cols + col] = acc[cc];
                }
            }
        }
    }
}

static void qkv_projection_kernel(
    const gpt_i8 *x,
    const gpt_i8 *wqkv,
    gpt_i32 *qkv,
    int tokens,
    int embd
) {
qkv_tok:
    for (int t = 0; t < tokens; t++) {
    qkv_col:
        for (int c = 0; c < 3 * embd; c++) {
#pragma HLS PIPELINE II=1
            gpt_i32 acc = 0;
        qkv_inner:
            for (int k = 0; k < embd; k++) {
#pragma HLS UNROLL factor=8
                acc += (gpt_i16)x[t * embd + k] * (gpt_i16)wqkv[c * embd + k];
            }
            qkv[t * (3 * embd) + c] = acc;
        }
    }
}

static void attention_score_kernel(
    const gpt_i8 *q,
    const gpt_i8 *k,
    gpt_i32 *score,
    int tokens,
    int head_size
) {
attn_i:
    for (int i = 0; i < tokens; i++) {
    attn_j:
        for (int j = 0; j < tokens; j++) {
#pragma HLS PIPELINE II=1
            gpt_i32 acc = 0;
        attn_d:
            for (int d = 0; d < head_size; d++) {
#pragma HLS UNROLL factor=8
                acc += (gpt_i16)q[i * head_size + d] * (gpt_i16)k[j * head_size + d];
            }
            if (j > i) {
                score[i * tokens + j] = -2147483647;
            } else {
                score[i * tokens + j] = acc;
            }
        }
    }
}

static void attention_apply_kernel(
    const gpt_i8 *q,
    const gpt_i8 *k,
    const gpt_i16 *v,
    gpt_i32 *out,
    int tokens,
    int head_size
) {
    static gpt_i32 scores[GPT_BLOCK_SIZE];
    static gpt_i16 probs[GPT_BLOCK_SIZE];
#pragma HLS BIND_STORAGE variable=scores type=ram_2p impl=bram
#pragma HLS BIND_STORAGE variable=probs type=ram_2p impl=bram

attn_apply_i:
    for (int i = 0; i < tokens; i++) {
        gpt_i32 max_score = -2147483647;
    score_j:
        for (int j = 0; j <= i; j++) {
#pragma HLS PIPELINE II=1
            gpt_i32 acc = 0;
        score_d:
            for (int d = 0; d < head_size; d++) {
#pragma HLS UNROLL factor=8
                acc += (gpt_i16)q[i * head_size + d] * (gpt_i16)k[j * head_size + d];
            }
            scores[j] = acc;
            if (acc > max_score) max_score = acc;
        }
        gpt_i32 psum = 0;
    prob_j:
        for (int j = 0; j <= i; j++) {
#pragma HLS PIPELINE II=1
            gpt_i16 p = exp_lut_approx((scores[j] - max_score) >> 2);
            probs[j] = p;
            psum += p;
        }
    value_d:
        for (int d = 0; d < head_size; d++) {
#pragma HLS PIPELINE II=1
            gpt_i32 acc = 0;
        value_j:
            for (int j = 0; j <= i; j++) {
#pragma HLS UNROLL factor=4
                acc += (gpt_i32)probs[j] * (gpt_i32)v[j * head_size + d];
            }
            gpt_i32 norm = 0;
            if (psum > 0) norm = (gpt_i32)(acc / psum);
            out[i * head_size + d] = norm;
        }
    }
}

static void kv_append_kernel(
    const gpt_i8 *new_k,
    const gpt_i8 *new_v,
    gpt_i16 *kv_cache,
    int pos,
    int embd
) {
    int base_k = pos * embd;
    int base_v = GPT_MAX_SUPPORTED_NEW_TOKENS * embd + pos * embd;
kv_loop:
    for (int i = 0; i < embd; i++) {
#pragma HLS PIPELINE II=1
        kv_cache[base_k + i] = new_k[i];
        kv_cache[base_v + i] = new_v[i];
    }
}

static void token_step_kernel(
    const gpt_i8 *x,
    const gpt_i8 *wqkv,
    gpt_i16 *kv_cache,
    gpt_i32 *out,
    int pos,
    int embd
) {
    static gpt_i8 q[GPT_N_EMBD];
    static gpt_i8 k[GPT_N_EMBD];
    static gpt_i8 v[GPT_N_EMBD];
    static gpt_i32 context[GPT_N_EMBD];
    static gpt_i32 scores[GPT_MAX_SUPPORTED_NEW_TOKENS];
    static gpt_i16 probs[GPT_MAX_SUPPORTED_NEW_TOKENS];
#pragma HLS ARRAY_PARTITION variable=q cyclic factor=8
#pragma HLS ARRAY_PARTITION variable=k cyclic factor=8
#pragma HLS ARRAY_PARTITION variable=v cyclic factor=8
#pragma HLS BIND_STORAGE variable=scores type=ram_2p impl=uram
#pragma HLS BIND_STORAGE variable=probs type=ram_2p impl=bram

qkv_one:
    for (int o = 0; o < 3 * embd; o++) {
#pragma HLS PIPELINE II=1
        gpt_i32 acc = 0;
    qkv_one_in:
        for (int i = 0; i < embd; i++) {
#pragma HLS UNROLL factor=8
                acc += (gpt_i16)x[i] * (gpt_i16)wqkv[o * embd + i];
        }
        gpt_i8 qv = (gpt_i8)(acc >> 8);
        if (o < embd) q[o] = qv;
        else if (o < 2 * embd) k[o - embd] = qv;
        else v[o - 2 * embd] = qv;
    }
    kv_append_kernel(k, v, kv_cache, pos, embd);

head_loop:
    for (int h = 0; h < GPT_N_HEAD; h++) {
        int hbase = h * GPT_HEAD_SIZE;
        gpt_i32 max_score = -2147483647;
    tok_score:
        for (int t = 0; t <= pos; t++) {
#pragma HLS PIPELINE II=1
            gpt_i32 acc = 0;
        tok_d:
            for (int d = 0; d < GPT_HEAD_SIZE; d++) {
#pragma HLS UNROLL factor=8
                acc += (gpt_i16)q[hbase + d] * kv_cache[t * embd + hbase + d];
            }
            scores[t] = acc;
            if (acc > max_score) max_score = acc;
        }
        gpt_i32 psum = 0;
    tok_prob:
        for (int t = 0; t <= pos; t++) {
#pragma HLS PIPELINE II=1
            gpt_i16 p = exp_lut_approx((scores[t] - max_score) >> 2);
            probs[t] = p;
            psum += p;
        }
    tok_ctx:
        for (int d = 0; d < GPT_HEAD_SIZE; d++) {
#pragma HLS PIPELINE II=1
            gpt_i32 acc = 0;
        tok_ctx_sum:
            for (int t = 0; t <= pos; t++) {
#pragma HLS UNROLL factor=4
                int vbase = GPT_MAX_SUPPORTED_NEW_TOKENS * embd + t * embd;
                acc += (gpt_i32)probs[t] * kv_cache[vbase + hbase + d];
            }
            gpt_i32 norm = 0;
            if (psum > 0) norm = (gpt_i32)(acc / psum);
            context[hbase + d] = norm;
        }
    }
copy_ctx:
    for (int i = 0; i < embd; i++) {
#pragma HLS PIPELINE II=1
        out[i] = context[i];
    }
}

static void mlp_kernel(
    const gpt_i8 *x,
    const gpt_i8 *w1,
    const gpt_i16 *w2,
    gpt_i32 *out,
    int tokens,
    int embd,
    int hidden
) {
    static gpt_i32 hidden_buf[GPT_MLP_HIDDEN];
#pragma HLS BIND_STORAGE variable=hidden_buf type=ram_2p impl=bram

mlp_tok:
    for (int t = 0; t < tokens; t++) {
    mlp_h:
        for (int h = 0; h < hidden; h++) {
#pragma HLS PIPELINE II=1
            gpt_i32 acc = 0;
        mlp_in:
            for (int k = 0; k < embd; k++) {
#pragma HLS UNROLL factor=8
                acc += (gpt_i16)x[t * embd + k] * (gpt_i16)w1[h * embd + k];
            }
            hidden_buf[h] = gelu_approx_i32(acc);
        }
    mlp_out:
        for (int c = 0; c < embd; c++) {
#pragma HLS PIPELINE II=1
            gpt_i32 acc = 0;
        mlp_proj:
            for (int h = 0; h < hidden; h++) {
#pragma HLS UNROLL factor=4
                acc += (hidden_buf[h] >> 8) * (gpt_i32)w2[c * hidden + h];
            }
            out[t * embd + c] = acc;
        }
    }
}

static void layernorm_kernel(
    const gpt_i8 *x,
    gpt_i32 *out,
    int tokens,
    int embd
) {
ln_tok:
    for (int t = 0; t < tokens; t++) {
        gpt_i32 sum = 0;
    ln_sum:
        for (int i = 0; i < embd; i++) {
#pragma HLS PIPELINE II=1
            sum += x[t * embd + i];
        }
        gpt_i32 mean = sum / embd;
    ln_out:
        for (int i = 0; i < embd; i++) {
#pragma HLS PIPELINE II=1
            out[t * embd + i] = ((gpt_i32)x[t * embd + i] - mean) << 8;
        }
    }
}

extern "C" {
void gpt_accel(
    const gpt_i8 *in_a,
    const gpt_i8 *in_b,
    gpt_i16 *in_c,
    gpt_i32 *out,
    int rows,
    int cols,
    int inner,
    int mode
) {
#pragma HLS INTERFACE m_axi port=in_a offset=slave bundle=gmem0 max_read_burst_length=128 num_read_outstanding=16
#pragma HLS INTERFACE m_axi port=in_b offset=slave bundle=gmem1 max_read_burst_length=128 num_read_outstanding=16
#pragma HLS INTERFACE m_axi port=in_c offset=slave bundle=gmem2 max_read_burst_length=128 max_write_burst_length=128 num_read_outstanding=16 num_write_outstanding=16
#pragma HLS INTERFACE m_axi port=out offset=slave bundle=gmem3 max_write_burst_length=128 num_write_outstanding=16
#pragma HLS INTERFACE s_axilite port=in_a bundle=control
#pragma HLS INTERFACE s_axilite port=in_b bundle=control
#pragma HLS INTERFACE s_axilite port=in_c bundle=control
#pragma HLS INTERFACE s_axilite port=out bundle=control
#pragma HLS INTERFACE s_axilite port=rows bundle=control
#pragma HLS INTERFACE s_axilite port=cols bundle=control
#pragma HLS INTERFACE s_axilite port=inner bundle=control
#pragma HLS INTERFACE s_axilite port=mode bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control
    if (mode == GPT_MODE_MATMUL) {
        matmul_int8_kernel(in_a, in_b, out, rows, cols, inner);
    } else if (mode == GPT_MODE_QKV) {
        qkv_projection_kernel(in_a, in_b, out, rows, inner);
    } else if (mode == GPT_MODE_ATTN_SCORE) {
        attention_score_kernel(in_a, in_b, out, rows, inner);
    } else if (mode == GPT_MODE_MLP) {
        mlp_kernel(in_a, in_b, in_c, out, rows, cols, inner);
    } else if (mode == GPT_MODE_LAYERNORM) {
        layernorm_kernel(in_a, out, rows, cols);
    } else if (mode == GPT_MODE_ATTN_APPLY) {
        attention_apply_kernel(in_a, in_b, in_c, out, rows, inner);
    } else if (mode == GPT_MODE_KV_APPEND) {
        kv_append_kernel(in_a, in_b, in_c, rows, cols);
    } else if (mode == GPT_MODE_TOKEN_STEP) {
        token_step_kernel(in_a, in_b, in_c, out, rows, cols);
    }
}
}
