#include "gpt_runtime.h"

#include "gpt_config.h"
#include "gpt_hw_accel.h"
#include "gpt_tokenizer.h"
#include "gpt_vocab.h"
#include "gpt_weights.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

#define GPT_TOP_K 25U
#define GPT_TEMPERATURE 0.55f
#define LN_EPS 1.0e-5f

typedef struct {
    const int8_t *c_attn_w;
    float c_attn_w_s;
    const int8_t *c_attn_b;
    float c_attn_b_s;
    const int8_t *c_proj_w;
    float c_proj_w_s;
    const int8_t *c_proj_b;
    float c_proj_b_s;
    const int8_t *ln1_w;
    float ln1_w_s;
    const int8_t *ln1_b;
    float ln1_b_s;
    const int8_t *ln2_w;
    float ln2_w_s;
    const int8_t *ln2_b;
    float ln2_b_s;
    const int8_t *fc_w;
    float fc_w_s;
    const int8_t *fc_b;
    float fc_b_s;
    const int8_t *mlp_proj_w;
    float mlp_proj_w_s;
    const int8_t *mlp_proj_b;
    float mlp_proj_b_s;
} BlockWeights;

static const BlockWeights g_blocks[GPT_N_LAYER] = {
    {
        GPTW_transformer_h_0_attn_c_attn_weight, 2.0643111289e-03f,
        GPTW_transformer_h_0_attn_c_attn_bias, 4.3664490144e-03f,
        GPTW_transformer_h_0_attn_c_proj_weight, 8.0736169196e-04f,
        GPTW_transformer_h_0_attn_c_proj_bias, 2.3558688915e-04f,
        GPTW_transformer_h_0_ln_1_weight, 8.7314562535e-03f,
        GPTW_transformer_h_0_ln_1_bias, 2.3756308818e-03f,
        GPTW_transformer_h_0_ln_2_weight, 1.0150393163e-02f,
        GPTW_transformer_h_0_ln_2_bias, 1.6243068252e-03f,
        GPTW_transformer_h_0_mlp_c_fc_weight, 1.5882921266e-03f,
        GPTW_transformer_h_0_mlp_c_fc_bias, 5.9840077256e-04f,
        GPTW_transformer_h_0_mlp_c_proj_weight, 9.4508948758e-04f,
        GPTW_transformer_h_0_mlp_c_proj_bias, 2.7788932047e-04f,
    },
    {
        GPTW_transformer_h_1_attn_c_attn_weight, 2.2153798051e-03f,
        GPTW_transformer_h_1_attn_c_attn_bias, 1.7772072882e-03f,
        GPTW_transformer_h_1_attn_c_proj_weight, 1.0884415916e-03f,
        GPTW_transformer_h_1_attn_c_proj_bias, 1.7309693370e-04f,
        GPTW_transformer_h_1_ln_1_weight, 8.9461259016e-03f,
        GPTW_transformer_h_1_ln_1_bias, 9.1361266187e-04f,
        GPTW_transformer_h_1_ln_2_weight, 1.0853155391e-02f,
        GPTW_transformer_h_1_ln_2_bias, 1.2382773433e-03f,
        GPTW_transformer_h_1_mlp_c_fc_weight, 1.9023497039e-03f,
        GPTW_transformer_h_1_mlp_c_fc_bias, 9.5119714502e-04f,
        GPTW_transformer_h_1_mlp_c_proj_weight, 1.8982687804e-03f,
        GPTW_transformer_h_1_mlp_c_proj_bias, 7.5492562037e-04f,
    },
};

static float g_k_cache[GPT_N_LAYER][GPT_BLOCK_SIZE][GPT_N_EMBD];
static float g_v_cache[GPT_N_LAYER][GPT_BLOCK_SIZE][GPT_N_EMBD];
static float g_x[GPT_N_EMBD];
static float g_norm[GPT_N_EMBD];
static float g_qkv[3 * GPT_N_EMBD];
static float g_attn_out[GPT_N_EMBD];
static float g_proj[GPT_N_EMBD];
static float g_mlp[GPT_MLP_HIDDEN];
static float g_scores[GPT_BLOCK_SIZE];
static float g_logits[GPT_VOCAB_SIZE];
static uint32_t g_rng_state = 1U;

static float deq(const int8_t *data, float scale, unsigned int idx)
{
    return (float)data[idx] * scale;
}

static void linear(GptHwLinearId hw_id, float *out, const float *in,
                   unsigned int rows, unsigned int cols,
                   const int8_t *w, float w_scale, const int8_t *b, float b_scale)
{
    if (hw_id != GPT_HW_LINEAR_NONE &&
        gpt_hw_linear_by_id(hw_id, out, in, rows, cols, w_scale, b, b_scale)) {
        return;
    }

    for (unsigned int r = 0U; r < rows; ++r) {
        float acc = b ? deq(b, b_scale, r) : 0.0f;
        const unsigned int base = r * cols;
        for (unsigned int c = 0U; c < cols; ++c) {
            acc += in[c] * deq(w, w_scale, base + c);
        }
        out[r] = acc;
    }
}

static void layernorm(float *out, const float *in,
                      const int8_t *weight, float weight_scale,
                      const int8_t *bias, float bias_scale)
{
    float mean = 0.0f;
    for (unsigned int i = 0U; i < GPT_N_EMBD; ++i) {
        mean += in[i];
    }
    mean /= (float)GPT_N_EMBD;

    float var = 0.0f;
    for (unsigned int i = 0U; i < GPT_N_EMBD; ++i) {
        const float d = in[i] - mean;
        var += d * d;
    }
    var /= (float)GPT_N_EMBD;
    const float inv_std = 1.0f / sqrtf(var + LN_EPS);

    for (unsigned int i = 0U; i < GPT_N_EMBD; ++i) {
        out[i] = (in[i] - mean) * inv_std * deq(weight, weight_scale, i) +
                 deq(bias, bias_scale, i);
    }
}

static float gelu_approx(float x)
{
    const float k0 = 0.7978845608f;
    const float k1 = 0.044715f;
    return 0.5f * x * (1.0f + tanhf(k0 * (x + k1 * x * x * x)));
}

static void softmax_inplace(float *x, unsigned int n)
{
    float max_v = x[0];
    for (unsigned int i = 1U; i < n; ++i) {
        if (x[i] > max_v) {
            max_v = x[i];
        }
    }
    float sum = 0.0f;
    for (unsigned int i = 0U; i < n; ++i) {
        x[i] = expf(x[i] - max_v);
        sum += x[i];
    }
    if (sum <= 0.0f) {
        const float uniform = 1.0f / (float)n;
        for (unsigned int i = 0U; i < n; ++i) {
            x[i] = uniform;
        }
        return;
    }
    const float inv_sum = 1.0f / sum;
    for (unsigned int i = 0U; i < n; ++i) {
        x[i] *= inv_sum;
    }
}

static void attention_one_token(const float *q, unsigned int layer, unsigned int pos)
{
    const float scale = 1.0f / sqrtf((float)GPT_HEAD_SIZE);
    for (unsigned int i = 0U; i < GPT_N_EMBD; ++i) {
        g_attn_out[i] = 0.0f;
    }

    for (unsigned int h = 0U; h < GPT_N_HEAD; ++h) {
        const unsigned int hbase = h * GPT_HEAD_SIZE;
        for (unsigned int t = 0U; t <= pos; ++t) {
            float score = 0.0f;
            for (unsigned int d = 0U; d < GPT_HEAD_SIZE; ++d) {
                score += q[hbase + d] * g_k_cache[layer][t][hbase + d];
            }
            g_scores[t] = score * scale;
        }
        softmax_inplace(g_scores, pos + 1U);
        for (unsigned int t = 0U; t <= pos; ++t) {
            const float p = g_scores[t];
            for (unsigned int d = 0U; d < GPT_HEAD_SIZE; ++d) {
                g_attn_out[hbase + d] += p * g_v_cache[layer][t][hbase + d];
            }
        }
    }
}

static void forward_token(int token, unsigned int pos, float *logits)
{
    if (token < 0 || token >= GPT_VOCAB_SIZE) {
        token = GPT_UNK_TOKEN;
    }
    if (pos >= GPT_BLOCK_SIZE) {
        pos = GPT_BLOCK_SIZE - 1U;
    }

    for (unsigned int i = 0U; i < GPT_N_EMBD; ++i) {
        g_x[i] = deq(GPTW_transformer_wte_weight, 1.6804450140e-03f,
                     (unsigned int)token * GPT_N_EMBD + i) +
                 deq(GPTW_transformer_wpe_weight, 5.0677610195e-03f,
                     pos * GPT_N_EMBD + i);
    }

    for (unsigned int l = 0U; l < GPT_N_LAYER; ++l) {
        const BlockWeights *bw = &g_blocks[l];

        layernorm(g_norm, g_x, bw->ln1_w, bw->ln1_w_s, bw->ln1_b, bw->ln1_b_s);
        const GptHwLinearId attn_id = (l == 0U) ? GPT_HW_LINEAR_H0_ATTN : GPT_HW_LINEAR_H1_ATTN;
        const GptHwLinearId attn_proj_id =
            (l == 0U) ? GPT_HW_LINEAR_H0_ATTN_PROJ : GPT_HW_LINEAR_H1_ATTN_PROJ;
        const GptHwLinearId mlp_fc_id = (l == 0U) ? GPT_HW_LINEAR_H0_MLP_FC : GPT_HW_LINEAR_H1_MLP_FC;
        const GptHwLinearId mlp_proj_id =
            (l == 0U) ? GPT_HW_LINEAR_H0_MLP_PROJ : GPT_HW_LINEAR_H1_MLP_PROJ;

        linear(attn_id, g_qkv, g_norm, 3U * GPT_N_EMBD, GPT_N_EMBD,
               bw->c_attn_w, bw->c_attn_w_s, bw->c_attn_b, bw->c_attn_b_s);

        for (unsigned int i = 0U; i < GPT_N_EMBD; ++i) {
            g_k_cache[l][pos][i] = g_qkv[GPT_N_EMBD + i];
            g_v_cache[l][pos][i] = g_qkv[2U * GPT_N_EMBD + i];
        }
        attention_one_token(g_qkv, l, pos);
        linear(attn_proj_id, g_proj, g_attn_out, GPT_N_EMBD, GPT_N_EMBD,
               bw->c_proj_w, bw->c_proj_w_s, bw->c_proj_b, bw->c_proj_b_s);
        for (unsigned int i = 0U; i < GPT_N_EMBD; ++i) {
            g_x[i] += g_proj[i];
        }

        layernorm(g_norm, g_x, bw->ln2_w, bw->ln2_w_s, bw->ln2_b, bw->ln2_b_s);
        linear(mlp_fc_id, g_mlp, g_norm, GPT_MLP_HIDDEN, GPT_N_EMBD,
               bw->fc_w, bw->fc_w_s, bw->fc_b, bw->fc_b_s);
        for (unsigned int i = 0U; i < GPT_MLP_HIDDEN; ++i) {
            g_mlp[i] = gelu_approx(g_mlp[i]);
        }
        linear(mlp_proj_id, g_proj, g_mlp, GPT_N_EMBD, GPT_MLP_HIDDEN,
               bw->mlp_proj_w, bw->mlp_proj_w_s, bw->mlp_proj_b, bw->mlp_proj_b_s);
        for (unsigned int i = 0U; i < GPT_N_EMBD; ++i) {
            g_x[i] += g_proj[i];
        }
    }

    layernorm(g_norm, g_x, GPTW_transformer_ln_f_weight, 1.2001058248e-02f,
              GPTW_transformer_ln_f_bias, 2.5228444986e-03f);
    linear(GPT_HW_LINEAR_LM_HEAD, logits, g_norm, GPT_VOCAB_SIZE, GPT_N_EMBD,
           GPTW_lm_head_weight, 3.8710780970e-03f, 0, 1.0f);
}

static void seed_rng(const char *s)
{
    uint32_t h = 2166136261U;
    while (*s != '\0') {
        h ^= (uint8_t)(*s++);
        h *= 16777619U;
    }
    g_rng_state = h ? h : 1U;
}

static uint32_t rng_next(void)
{
    g_rng_state = g_rng_state * 1664525U + 1013904223U;
    return g_rng_state;
}

static int sample_next_token(const float *logits)
{
    int top_idx[GPT_TOP_K];
    float top_val[GPT_TOP_K];
    for (unsigned int i = 0U; i < GPT_TOP_K; ++i) {
        top_idx[i] = -1;
        top_val[i] = -3.4e38f;
    }

    for (int t = 0; t < GPT_VOCAB_SIZE; ++t) {
        const float v = logits[t];
        for (unsigned int k = 0U; k < GPT_TOP_K; ++k) {
            if (v > top_val[k]) {
                for (unsigned int m = GPT_TOP_K - 1U; m > k; --m) {
                    top_val[m] = top_val[m - 1U];
                    top_idx[m] = top_idx[m - 1U];
                }
                top_val[k] = v;
                top_idx[k] = t;
                break;
            }
        }
    }

    const float temp = GPT_TEMPERATURE;
    float max_v = top_val[0] / temp;
    float sum = 0.0f;
    for (unsigned int k = 0U; k < GPT_TOP_K; ++k) {
        if (top_idx[k] < 0) {
            g_scores[k] = 0.0f;
            continue;
        }
        g_scores[k] = expf((top_val[k] / temp) - max_v);
        sum += g_scores[k];
    }
    if (sum <= 0.0f) {
        return top_idx[0] >= 0 ? top_idx[0] : GPT_UNK_TOKEN;
    }

    const float r = ((float)(rng_next() & 0x00ffffffU) / 16777216.0f) * sum;
    float acc = 0.0f;
    for (unsigned int k = 0U; k < GPT_TOP_K; ++k) {
        acc += g_scores[k];
        if (r <= acc && top_idx[k] >= 0) {
            return top_idx[k];
        }
    }
    return top_idx[0] >= 0 ? top_idx[0] : GPT_UNK_TOKEN;
}

static void build_model_prompt(const char *prompt, char *dst, unsigned int cap)
{
    const char *prefix = "User: ";
    const char *suffix = "\nAssistant:";
    unsigned int w = 0U;
    for (unsigned int i = 0U; prefix[i] != '\0' && w + 1U < cap; ++i) {
        dst[w++] = prefix[i];
    }
    for (unsigned int i = 0U; prompt[i] != '\0' && w + 1U < cap; ++i) {
        dst[w++] = prompt[i];
    }
    for (unsigned int i = 0U; suffix[i] != '\0' && w + 1U < cap; ++i) {
        dst[w++] = suffix[i];
    }
    dst[w] = '\0';
}

static void rebuild_context(const int *context, unsigned int ctx_len, float *logits)
{
    memset(g_k_cache, 0, sizeof(g_k_cache));
    memset(g_v_cache, 0, sizeof(g_v_cache));
    for (unsigned int i = 0U; i < ctx_len; ++i) {
        forward_token(context[i], i, logits);
    }
}

static void shift_kv_cache_left(void)
{
    for (unsigned int l = 0U; l < GPT_N_LAYER; ++l) {
        memmove(g_k_cache[l], g_k_cache[l] + 1,
                sizeof(g_k_cache[l][0]) * (GPT_BLOCK_SIZE - 1U));
        memmove(g_v_cache[l], g_v_cache[l] + 1,
                sizeof(g_v_cache[l][0]) * (GPT_BLOCK_SIZE - 1U));
        memset(g_k_cache[l][GPT_BLOCK_SIZE - 1U], 0, sizeof(g_k_cache[l][0]));
        memset(g_v_cache[l][GPT_BLOCK_SIZE - 1U], 0, sizeof(g_v_cache[l][0]));
    }
}

int gpt_generate(const char *prompt, unsigned int max_new_tokens, char *out_text, unsigned int out_cap)
{
    int context[GPT_BLOCK_SIZE];
    unsigned int ctx_len = 0U;
    char formatted[224];

    if (out_cap == 0U) {
        return 0;
    }
    build_model_prompt(prompt, formatted, sizeof(formatted));
    seed_rng(formatted);

    for (unsigned int i = 0U; formatted[i] != '\0'; ++i) {
        if (ctx_len == GPT_BLOCK_SIZE) {
            memmove(context, context + 1, sizeof(context[0]) * (GPT_BLOCK_SIZE - 1U));
            ctx_len = GPT_BLOCK_SIZE - 1U;
        }
        context[ctx_len++] = gpt_encode_char(formatted[i]);
    }
    if (ctx_len == 0U) {
        context[ctx_len++] = gpt_encode_char(' ');
    }

    rebuild_context(context, ctx_len, g_logits);

    unsigned int produced = 0U;
    while (produced < max_new_tokens && produced + 1U < out_cap) {
        int token = sample_next_token(g_logits);
        char ch = gpt_decode_token(token);
        out_text[produced++] = ch;

        if (ctx_len == GPT_BLOCK_SIZE) {
            memmove(context, context + 1, sizeof(context[0]) * (GPT_BLOCK_SIZE - 1U));
            context[GPT_BLOCK_SIZE - 1U] = token;
            shift_kv_cache_left();
            forward_token(token, GPT_BLOCK_SIZE - 1U, g_logits);
            ctx_len = GPT_BLOCK_SIZE;
        } else {
            context[ctx_len] = token;
            forward_token(token, ctx_len, g_logits);
            ++ctx_len;
        }
    }
    out_text[produced] = '\0';
    return (int)produced;
}
