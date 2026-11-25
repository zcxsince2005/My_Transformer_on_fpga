#ifndef GPT_PERF_H
#define GPT_PERF_H

#include "platform_config.h"

typedef struct {
    char prompt[160];
    char generated[GPT_APP_MAX_NEW_TOKENS + 1];
    unsigned int token_count;
    unsigned long long total_cycles;
    unsigned long long token_cycles[GPT_APP_MAX_NEW_TOKENS];
    unsigned int fallback_used;
} GptPerf;

extern GptPerf g_gpt_perf;

void gpt_perf_reset(void);
void gpt_perf_print(void);

#endif
