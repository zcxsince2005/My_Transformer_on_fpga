#ifndef GPT_TEST_VECTORS_H
#define GPT_TEST_VECTORS_H

#include "gpt_config.h"

#define GPT_TEST_PROMPT_LEN 15
#define GPT_TEST_OUTPUT_LEN 31
static const int32_t GPT_TEST_PROMPT[15] = {
    85, 115, 101, 114, 58, 32, 104, 101, 108, 108, 111, 10,
    65, 73, 58,
};

static const int32_t GPT_TEST_OUTPUT[31] = {
    85, 115, 101, 114, 58, 32, 104, 101, 108, 108, 111, 10,
    65, 73, 58, 32, 72, 101, 108, 108, 111, 46, 32, 73,
    32, 97, 109, 32, 97, 32, 116,
};

#endif
