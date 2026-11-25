#ifndef GPT_ACCEL_H
#define GPT_ACCEL_H

#include <ap_fixed.h>
#include <ap_int.h>
#include <stdint.h>

#include "gpt_config.h"

typedef ap_int<8> gpt_i8;
typedef ap_int<16> gpt_i16;
typedef ap_int<32> gpt_i32;
typedef ap_fixed<24, 8> gpt_fixed;

#define GPT_MODE_MATMUL 0
#define GPT_MODE_QKV 1
#define GPT_MODE_ATTN_SCORE 2
#define GPT_MODE_MLP 3
#define GPT_MODE_LAYERNORM 4
#define GPT_MODE_ATTN_APPLY 5
#define GPT_MODE_KV_APPEND 6
#define GPT_MODE_TOKEN_STEP 7
#define GPT_LINEAR_PARALLEL_FACTOR 64
#define GPT_MATMUL_OUT_PARALLEL_FACTOR 4

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
);
}

#endif
