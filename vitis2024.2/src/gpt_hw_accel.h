#ifndef GPT_HW_ACCEL_H
#define GPT_HW_ACCEL_H

#include <stdint.h>

#define GPT_V4_HLS_LINEAR_PARALLEL_FACTOR 64U
#define GPT_V4_HLS_MATMUL_OUT_PARALLEL_FACTOR 4U

typedef enum {
    GPT_HW_LINEAR_H0_ATTN = 0,
    GPT_HW_LINEAR_H0_ATTN_PROJ,
    GPT_HW_LINEAR_H0_MLP_FC,
    GPT_HW_LINEAR_H0_MLP_PROJ,
    GPT_HW_LINEAR_H1_ATTN,
    GPT_HW_LINEAR_H1_ATTN_PROJ,
    GPT_HW_LINEAR_H1_MLP_FC,
    GPT_HW_LINEAR_H1_MLP_PROJ,
    GPT_HW_LINEAR_LM_HEAD,
    GPT_HW_LINEAR_NONE
} GptHwLinearId;

int gpt_hw_linear(float *out, const float *in, unsigned int rows, unsigned int cols,
                  const int8_t *w, float w_scale, const int8_t *b, float b_scale);
int gpt_hw_linear_by_id(GptHwLinearId id, float *out, const float *in,
                        unsigned int rows, unsigned int cols,
                        float w_scale, const int8_t *b, float b_scale);
void gpt_hw_reset_token_cache(void);
void gpt_hw_shift_token_cache_left(void);
int gpt_hw_attention_step(unsigned int layer, float *out, const float *norm,
                          unsigned int pos, float w_scale);
unsigned int gpt_hw_linear_calls(void);
unsigned int gpt_hw_linear_fallbacks(void);
unsigned int gpt_hw_token_step_calls(void);
unsigned int gpt_hw_block_step_calls(void);
unsigned int gpt_hw_cache_flush_count(void);
unsigned int gpt_hw_cache_invalidate_count(void);
const char *gpt_hw_mode_name(void);

#endif
