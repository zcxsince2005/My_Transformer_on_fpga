#include "gpt_perf.h"

#include "gpt_config.h"
#include "gpt_hw_accel.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xtime_l.h"
#include <string.h>

GptPerf g_gpt_perf;

void gpt_perf_reset(void) {
    memset(&g_gpt_perf, 0, sizeof(g_gpt_perf));
}

void gpt_perf_print(void) {
    if (g_gpt_perf.token_count == 0U) {
        xil_printf("No tiny GPT generation has been run yet.\r\n");
        return;
    }
    unsigned long long ms = (g_gpt_perf.total_cycles * 1000ULL) / COUNTS_PER_SECOND;
    unsigned long long avg_us = (g_gpt_perf.total_cycles * 1000000ULL) /
                                ((unsigned long long)COUNTS_PER_SECOND * g_gpt_perf.token_count);
    xil_printf("Prompt: %s\r\n", g_gpt_perf.prompt);
    xil_printf("Generated: %s\r\n", g_gpt_perf.generated);
    xil_printf("Tokens: %u\r\n", g_gpt_perf.token_count);
    xil_printf("Total: %llu cycles, %llu ms\r\n", g_gpt_perf.total_cycles, ms);
    xil_printf("Average: %llu us/token\r\n", avg_us);
    xil_printf("Per-token cycles:");
    for (unsigned int i = 0; i < g_gpt_perf.token_count; ++i) {
        xil_printf(" %llu", g_gpt_perf.token_cycles[i]);
    }
    xil_printf("\r\n");
    xil_printf("Fallback used: %s\r\n", g_gpt_perf.fallback_used ? "yes" : "no");
    xil_printf("GPT mode: %s\r\n", gpt_hw_mode_name());
    xil_printf("GPT V4 n_embd: %u\r\n", GPT_N_EMBD);
    xil_printf("GPT V4 n_head: %u\r\n", GPT_N_HEAD);
    xil_printf("GPT V4 n_layer: %u\r\n", GPT_N_LAYER);
    xil_printf("GPT V4 mlp_hidden: %u\r\n", GPT_MLP_HIDDEN);
    xil_printf("GPT V4 HLS linear parallel factor: %u\r\n", GPT_V4_HLS_LINEAR_PARALLEL_FACTOR);
    xil_printf("GPT V4 HLS matmul output parallel factor: %u\r\n", GPT_V4_HLS_MATMUL_OUT_PARALLEL_FACTOR);
    xil_printf("GPT HLS token_step calls: %u\r\n", gpt_hw_token_step_calls());
    xil_printf("GPT HLS block_step calls: %u\r\n", gpt_hw_block_step_calls());
    xil_printf("GPT HLS linear calls: %u\r\n", gpt_hw_linear_calls());
    xil_printf("GPT HLS linear fallbacks: %u\r\n", gpt_hw_linear_fallbacks());
    xil_printf("Cache flush count: %u\r\n", gpt_hw_cache_flush_count());
    xil_printf("Cache invalidate count: %u\r\n", gpt_hw_cache_invalidate_count());
}
