#include "gpt_hw_accel.h"

#include "gpt_config.h"
#include "gpt_weights.h"
#include "platform_config.h"
#include "xil_cache.h"
#include "xil_io.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define GPT_ACCEL_AP_CTRL 0x00U
#define GPT_ACCEL_IN_A    0x10U
#define GPT_ACCEL_IN_B    0x1cU
#define GPT_ACCEL_IN_C    0x28U
#define GPT_ACCEL_OUT_R   0x34U
#define GPT_ACCEL_ROWS    0x40U
#define GPT_ACCEL_COLS    0x48U
#define GPT_ACCEL_INNER   0x50U
#define GPT_ACCEL_MODE    0x58U
#define GPT_MODE_MATMUL   0U
#define GPT_MODE_TOKEN_STEP 7U
#define GPT_HW_TIMEOUT    10000000U

#define ALIGN64 __attribute__((aligned(64)))

static int8_t g_hw_in[GPT_MLP_HIDDEN] ALIGN64;
static int16_t g_hw_dummy_c[8] ALIGN64;
static int32_t g_hw_out[GPT_MLP_HIDDEN] ALIGN64;
static int16_t g_hw_kv_cache[GPT_N_LAYER][2U * GPT_MAX_SUPPORTED_NEW_TOKENS * GPT_N_EMBD] ALIGN64;

static int8_t g_c_attn_w_t[GPT_N_LAYER][GPT_N_EMBD * (3 * GPT_N_EMBD)] ALIGN64;
static int8_t g_c_proj_w_t[GPT_N_LAYER][GPT_N_EMBD * GPT_N_EMBD] ALIGN64;
static int8_t g_fc_w_t[GPT_N_LAYER][GPT_N_EMBD * GPT_MLP_HIDDEN] ALIGN64;
static int8_t g_mlp_proj_w_t[GPT_N_LAYER][GPT_MLP_HIDDEN * GPT_N_EMBD] ALIGN64;
static int8_t g_lm_head_w_t[GPT_N_EMBD * GPT_VOCAB_SIZE] ALIGN64;

static unsigned int g_initialized = 0U;
static unsigned int g_calls = 0U;
static unsigned int g_fallbacks = 0U;
static unsigned int g_token_step_calls = 0U;
static unsigned int g_block_step_calls = 0U;
static unsigned int g_cache_flushes = 0U;
static unsigned int g_cache_invalidates = 0U;

static const int8_t *attn_weight_by_layer(unsigned int layer)
{
    if (layer == 0U) {
        return GPTW_transformer_h_0_attn_c_attn_weight;
    }
    if (layer == 1U) {
        return GPTW_transformer_h_1_attn_c_attn_weight;
    }
    return NULL;
}

static void transpose_i8(int8_t *dst, const int8_t *src, unsigned int rows, unsigned int cols)
{
    for (unsigned int r = 0U; r < rows; ++r) {
        for (unsigned int c = 0U; c < cols; ++c) {
            dst[c * rows + r] = src[r * cols + c];
        }
    }
}

static void init_weight_cache(void)
{
    if (g_initialized) {
        return;
    }

    transpose_i8(g_c_attn_w_t[0], GPTW_transformer_h_0_attn_c_attn_weight,
                 3U * GPT_N_EMBD, GPT_N_EMBD);
    transpose_i8(g_c_attn_w_t[1], GPTW_transformer_h_1_attn_c_attn_weight,
                 3U * GPT_N_EMBD, GPT_N_EMBD);
    transpose_i8(g_c_proj_w_t[0], GPTW_transformer_h_0_attn_c_proj_weight,
                 GPT_N_EMBD, GPT_N_EMBD);
    transpose_i8(g_c_proj_w_t[1], GPTW_transformer_h_1_attn_c_proj_weight,
                 GPT_N_EMBD, GPT_N_EMBD);
    transpose_i8(g_fc_w_t[0], GPTW_transformer_h_0_mlp_c_fc_weight,
                 GPT_MLP_HIDDEN, GPT_N_EMBD);
    transpose_i8(g_fc_w_t[1], GPTW_transformer_h_1_mlp_c_fc_weight,
                 GPT_MLP_HIDDEN, GPT_N_EMBD);
    transpose_i8(g_mlp_proj_w_t[0], GPTW_transformer_h_0_mlp_c_proj_weight,
                 GPT_N_EMBD, GPT_MLP_HIDDEN);
    transpose_i8(g_mlp_proj_w_t[1], GPTW_transformer_h_1_mlp_c_proj_weight,
                 GPT_N_EMBD, GPT_MLP_HIDDEN);
    transpose_i8(g_lm_head_w_t, GPTW_lm_head_weight, GPT_VOCAB_SIZE, GPT_N_EMBD);

    Xil_DCacheFlushRange((INTPTR)g_c_attn_w_t, sizeof(g_c_attn_w_t));
    Xil_DCacheFlushRange((INTPTR)g_c_proj_w_t, sizeof(g_c_proj_w_t));
    Xil_DCacheFlushRange((INTPTR)g_fc_w_t, sizeof(g_fc_w_t));
    Xil_DCacheFlushRange((INTPTR)g_mlp_proj_w_t, sizeof(g_mlp_proj_w_t));
    Xil_DCacheFlushRange((INTPTR)g_lm_head_w_t, sizeof(g_lm_head_w_t));
    g_cache_flushes += 5U;

    g_initialized = 1U;
}

static const int8_t *find_transposed_weight_by_id(GptHwLinearId id,
                                                  unsigned int rows,
                                                  unsigned int cols)
{
    switch (id) {
    case GPT_HW_LINEAR_H0_ATTN:
        return (rows == 3U * GPT_N_EMBD && cols == GPT_N_EMBD) ? g_c_attn_w_t[0] : NULL;
    case GPT_HW_LINEAR_H1_ATTN:
        return (rows == 3U * GPT_N_EMBD && cols == GPT_N_EMBD) ? g_c_attn_w_t[1] : NULL;
    case GPT_HW_LINEAR_H0_ATTN_PROJ:
        return (rows == GPT_N_EMBD && cols == GPT_N_EMBD) ? g_c_proj_w_t[0] : NULL;
    case GPT_HW_LINEAR_H1_ATTN_PROJ:
        return (rows == GPT_N_EMBD && cols == GPT_N_EMBD) ? g_c_proj_w_t[1] : NULL;
    case GPT_HW_LINEAR_H0_MLP_FC:
        return (rows == GPT_MLP_HIDDEN && cols == GPT_N_EMBD) ? g_fc_w_t[0] : NULL;
    case GPT_HW_LINEAR_H1_MLP_FC:
        return (rows == GPT_MLP_HIDDEN && cols == GPT_N_EMBD) ? g_fc_w_t[1] : NULL;
    case GPT_HW_LINEAR_H0_MLP_PROJ:
        return (rows == GPT_N_EMBD && cols == GPT_MLP_HIDDEN) ? g_mlp_proj_w_t[0] : NULL;
    case GPT_HW_LINEAR_H1_MLP_PROJ:
        return (rows == GPT_N_EMBD && cols == GPT_MLP_HIDDEN) ? g_mlp_proj_w_t[1] : NULL;
    case GPT_HW_LINEAR_LM_HEAD:
        return (rows == GPT_VOCAB_SIZE && cols == GPT_N_EMBD) ? g_lm_head_w_t : NULL;
    default:
        return NULL;
    }
}

static void write_addr64(uint32_t reg, uintptr_t ptr)
{
    Xil_Out32(GPT_ACCEL_BASEADDR + reg, (uint32_t)(ptr & 0xffffffffU));
    Xil_Out32(GPT_ACCEL_BASEADDR + reg + 4U, (uint32_t)((uint64_t)ptr >> 32));
}

static int wait_done(void)
{
    for (unsigned int i = 0U; i < GPT_HW_TIMEOUT; ++i) {
        uint32_t ctrl = Xil_In32(GPT_ACCEL_BASEADDR + GPT_ACCEL_AP_CTRL);
        if ((ctrl & 0x2U) != 0U) {
            return 1;
        }
    }
    return 0;
}

void gpt_hw_reset_token_cache(void)
{
    memset(g_hw_kv_cache, 0, sizeof(g_hw_kv_cache));
    Xil_DCacheFlushRange((INTPTR)g_hw_kv_cache, sizeof(g_hw_kv_cache));
    ++g_cache_flushes;
}

void gpt_hw_shift_token_cache_left(void)
{
    const unsigned int one = GPT_N_EMBD;
    const unsigned int max_tokens = GPT_MAX_SUPPORTED_NEW_TOKENS;
    for (unsigned int l = 0U; l < GPT_N_LAYER; ++l) {
        int16_t *kv = g_hw_kv_cache[l];
        memmove(kv, kv + one, (max_tokens - 1U) * one * sizeof(kv[0]));
        memset(kv + (max_tokens - 1U) * one, 0, one * sizeof(kv[0]));
        memmove(kv + max_tokens * one, kv + (max_tokens + 1U) * one,
                (max_tokens - 1U) * one * sizeof(kv[0]));
        memset(kv + (2U * max_tokens - 1U) * one, 0, one * sizeof(kv[0]));
    }
    Xil_DCacheFlushRange((INTPTR)g_hw_kv_cache, sizeof(g_hw_kv_cache));
    ++g_cache_flushes;
}

int gpt_hw_attention_step(unsigned int layer, float *out, const float *norm,
                          unsigned int pos, float w_scale)
{
    if (layer >= GPT_N_LAYER || pos >= GPT_BLOCK_SIZE) {
        ++g_fallbacks;
        return 0;
    }

    const int8_t *wqkv = attn_weight_by_layer(layer);
    if (wqkv == NULL) {
        ++g_fallbacks;
        return 0;
    }

    float max_abs = 0.0f;
    for (unsigned int i = 0U; i < GPT_N_EMBD; ++i) {
        float v = fabsf(norm[i]);
        if (v > max_abs) {
            max_abs = v;
        }
    }
    if (max_abs < 1.0e-9f) {
        memset(out, 0, GPT_N_EMBD * sizeof(out[0]));
        ++g_token_step_calls;
        return 1;
    }

    const float in_scale = max_abs / 127.0f;
    const float inv_in_scale = 1.0f / in_scale;
    for (unsigned int i = 0U; i < GPT_N_EMBD; ++i) {
        int q = (int)lrintf(norm[i] * inv_in_scale);
        if (q > 127) q = 127;
        if (q < -128) q = -128;
        g_hw_in[i] = (int8_t)q;
    }

    int16_t *kv = g_hw_kv_cache[layer];
    Xil_DCacheFlushRange((INTPTR)g_hw_in, GPT_N_EMBD * sizeof(g_hw_in[0]));
    Xil_DCacheFlushRange((INTPTR)wqkv, (3U * GPT_N_EMBD * GPT_N_EMBD) * sizeof(wqkv[0]));
    Xil_DCacheFlushRange((INTPTR)kv, (2U * GPT_MAX_SUPPORTED_NEW_TOKENS * GPT_N_EMBD) * sizeof(kv[0]));
    Xil_DCacheFlushRange((INTPTR)g_hw_out, GPT_N_EMBD * sizeof(g_hw_out[0]));
    g_cache_flushes += 4U;

    write_addr64(GPT_ACCEL_IN_A, (uintptr_t)g_hw_in);
    write_addr64(GPT_ACCEL_IN_B, (uintptr_t)wqkv);
    write_addr64(GPT_ACCEL_IN_C, (uintptr_t)kv);
    write_addr64(GPT_ACCEL_OUT_R, (uintptr_t)g_hw_out);
    Xil_Out32(GPT_ACCEL_BASEADDR + GPT_ACCEL_ROWS, pos);
    Xil_Out32(GPT_ACCEL_BASEADDR + GPT_ACCEL_COLS, GPT_N_EMBD);
    Xil_Out32(GPT_ACCEL_BASEADDR + GPT_ACCEL_INNER, GPT_N_EMBD);
    Xil_Out32(GPT_ACCEL_BASEADDR + GPT_ACCEL_MODE, GPT_MODE_TOKEN_STEP);
    Xil_Out32(GPT_ACCEL_BASEADDR + GPT_ACCEL_AP_CTRL, 1U);

    if (!wait_done()) {
        ++g_fallbacks;
        return 0;
    }

    Xil_DCacheInvalidateRange((INTPTR)g_hw_out, GPT_N_EMBD * sizeof(g_hw_out[0]));
    Xil_DCacheInvalidateRange((INTPTR)kv, (2U * GPT_MAX_SUPPORTED_NEW_TOKENS * GPT_N_EMBD) * sizeof(kv[0]));
    g_cache_invalidates += 2U;

    const float out_scale = 256.0f * in_scale * w_scale;
    for (unsigned int i = 0U; i < GPT_N_EMBD; ++i) {
        out[i] = (float)g_hw_out[i] * out_scale;
    }
    ++g_token_step_calls;
    return 1;
}

int gpt_hw_linear_by_id(GptHwLinearId id, float *out, const float *in,
                        unsigned int rows, unsigned int cols,
                        float w_scale, const int8_t *b, float b_scale)
{
    if (rows == 0U || cols == 0U || rows > GPT_MLP_HIDDEN || cols > GPT_MLP_HIDDEN) {
        ++g_fallbacks;
        return 0;
    }

    init_weight_cache();
    const int8_t *wt = find_transposed_weight_by_id(id, rows, cols);
    if (wt == NULL) {
        ++g_fallbacks;
        return 0;
    }

    float max_abs = 0.0f;
    for (unsigned int i = 0U; i < cols; ++i) {
        float v = fabsf(in[i]);
        if (v > max_abs) {
            max_abs = v;
        }
    }
    if (max_abs < 1.0e-9f) {
        for (unsigned int r = 0U; r < rows; ++r) {
            out[r] = b ? ((float)b[r] * b_scale) : 0.0f;
        }
        ++g_calls;
        return 1;
    }

    const float in_scale = max_abs / 127.0f;
    const float inv_in_scale = 1.0f / in_scale;
    for (unsigned int i = 0U; i < cols; ++i) {
        int q = (int)lrintf(in[i] * inv_in_scale);
        if (q > 127) q = 127;
        if (q < -128) q = -128;
        g_hw_in[i] = (int8_t)q;
    }

    Xil_DCacheFlushRange((INTPTR)g_hw_in, cols * sizeof(g_hw_in[0]));
    Xil_DCacheFlushRange((INTPTR)g_hw_out, rows * sizeof(g_hw_out[0]));
    g_cache_flushes += 2U;

    write_addr64(GPT_ACCEL_IN_A, (uintptr_t)g_hw_in);
    write_addr64(GPT_ACCEL_IN_B, (uintptr_t)wt);
    write_addr64(GPT_ACCEL_IN_C, (uintptr_t)g_hw_dummy_c);
    write_addr64(GPT_ACCEL_OUT_R, (uintptr_t)g_hw_out);
    Xil_Out32(GPT_ACCEL_BASEADDR + GPT_ACCEL_ROWS, 1U);
    Xil_Out32(GPT_ACCEL_BASEADDR + GPT_ACCEL_COLS, rows);
    Xil_Out32(GPT_ACCEL_BASEADDR + GPT_ACCEL_INNER, cols);
    Xil_Out32(GPT_ACCEL_BASEADDR + GPT_ACCEL_MODE, GPT_MODE_MATMUL);
    Xil_Out32(GPT_ACCEL_BASEADDR + GPT_ACCEL_AP_CTRL, 1U);

    if (!wait_done()) {
        ++g_fallbacks;
        return 0;
    }
    Xil_DCacheInvalidateRange((INTPTR)g_hw_out, rows * sizeof(g_hw_out[0]));
    ++g_cache_invalidates;

    const float out_scale = in_scale * w_scale;
    for (unsigned int r = 0U; r < rows; ++r) {
        float acc = (float)g_hw_out[r] * out_scale;
        if (b) {
            acc += (float)b[r] * b_scale;
        }
        out[r] = acc;
    }
    ++g_calls;
    return 1;
}

int gpt_hw_linear(float *out, const float *in, unsigned int rows, unsigned int cols,
                  const int8_t *w, float w_scale, const int8_t *b, float b_scale)
{
    (void)out;
    (void)in;
    (void)rows;
    (void)cols;
    (void)w;
    (void)w_scale;
    (void)b;
    (void)b_scale;
    ++g_fallbacks;
    return 0;
}

unsigned int gpt_hw_linear_calls(void)
{
    return g_calls;
}

unsigned int gpt_hw_linear_fallbacks(void)
{
    return g_fallbacks;
}

unsigned int gpt_hw_token_step_calls(void)
{
    return g_token_step_calls;
}

unsigned int gpt_hw_block_step_calls(void)
{
    return g_block_step_calls;
}

unsigned int gpt_hw_cache_flush_count(void)
{
    return g_cache_flushes;
}

unsigned int gpt_hw_cache_invalidate_count(void)
{
    return g_cache_invalidates;
}

const char *gpt_hw_mode_name(void)
{
    return (g_token_step_calls > 0U) ? "TOKEN_STEP_ATTENTION_V1" : "LINEAR_ROLLING_KV";
}
