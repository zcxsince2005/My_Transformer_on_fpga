#include "gpt_dialog.h"

#include "gpt_perf.h"
#include "gpt_runtime.h"
#include "platform_config.h"
#include "uart_io.h"
#include "xil_printf.h"
#include "xtime_l.h"
#include <stdlib.h>
#include <string.h>

static unsigned int parse_len(const char *s)
{
    unsigned int n = (unsigned int)atoi(s);
    if (n < GPT_APP_MIN_NEW_TOKENS) {
        n = GPT_APP_MIN_NEW_TOKENS;
    }
    if (n > GPT_APP_MAX_NEW_TOKENS) {
        n = GPT_APP_MAX_NEW_TOKENS;
    }
    return n;
}

static char to_lower_ascii(char c)
{
    if (c >= 'A' && c <= 'Z') {
        return (char)(c + ('a' - 'A'));
    }
    return c;
}

static void normalize_input(char *s)
{
    unsigned int r = 0U;
    unsigned int w = 0U;
    while (s[r] == ' ' || s[r] == '\t') {
        ++r;
    }
    for (; s[r] != '\0'; ++r) {
        unsigned char c = (unsigned char)s[r];
        if (c >= 32U && c <= 126U) {
            s[w++] = (char)c;
        }
    }
    while (w > 0U && (s[w - 1U] == ' ' || s[w - 1U] == '\t')) {
        --w;
    }
    s[w] = '\0';
}

static int equals_ci(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        if (to_lower_ascii(*a) != to_lower_ascii(*b)) {
            return 0;
        }
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static int starts_with_ci(const char *s, const char *prefix)
{
    while (*prefix != '\0') {
        if (*s == '\0' || to_lower_ascii(*s) != to_lower_ascii(*prefix)) {
            return 0;
        }
        ++s;
        ++prefix;
    }
    return 1;
}

static int starts_marker(const char *s, const char *marker)
{
    while (*marker != '\0') {
        if (*s == '\0' || *s != *marker) {
            return 0;
        }
        ++s;
        ++marker;
    }
    return 1;
}

static int is_allowed_output_char(char c)
{
    return (c == '\n') ||
           (c >= 32 && c <= 126);
}

static unsigned int clean_completion(const char *raw, char *clean, unsigned int cap)
{
    static const char *markers[] = {
        "\nUser:",
        "\nHuman:",
        "\nQuestion:",
        "\nInstruction:",
        "\nSystem:",
        "\nResponse:",
        "\nAnswer:",
        "\nAI:",
    };
    unsigned int out = 0U;
    unsigned int spaces = 0U;

    for (unsigned int i = 0U; raw[i] != '\0' && out + 1U < cap; ++i) {
        int stop = 0;
        for (unsigned int m = 0U; m < sizeof(markers) / sizeof(markers[0]); ++m) {
            if (starts_marker(&raw[i], markers[m])) {
                stop = 1;
                break;
            }
        }
        if (stop) {
            break;
        }

        char c = raw[i];
        if (!is_allowed_output_char(c)) {
            c = ' ';
        }
        if (c == '\r' || c == '\t') {
            c = ' ';
        }
        if (c == '\n') {
            c = ' ';
        }
        if (c == ' ') {
            if (out == 0U || spaces >= 1U) {
                continue;
            }
            ++spaces;
        } else {
            spaces = 0U;
        }
        clean[out++] = c;
    }

    while (out > 0U && clean[out - 1U] == ' ') {
        --out;
    }
    clean[out] = '\0';

    if (starts_with_ci(clean, "assistant:")) {
        unsigned int i = 10U;
        while (clean[i] == ' ') {
            ++i;
        }
        memmove(clean, clean + i, strlen(clean + i) + 1U);
        out = (unsigned int)strlen(clean);
    }
    return out;
}

static void copy_reply(char *dst, unsigned int cap, const char *src)
{
    if (cap == 0U) {
        return;
    }
    strncpy(dst, src, cap - 1U);
    dst[cap - 1U] = '\0';
}

void gpt_dialog_run(void)
{
    char line[192];
    unsigned int max_tokens = GPT_APP_DEFAULT_NEW_TOKENS;

    xil_printf("Tiny GPT dialogue. Type help for commands. Type exit to return.\r\n");
    while (1) {
        xil_printf("User: ");
        uart_readline(line, sizeof(line));
        normalize_input(line);
        if (equals_ci(line, "exit")) {
            return;
        }
        if (equals_ci(line, "help")) {
            xil_printf("Commands: len 64, len 100, len 128, len 200; exit returns to menu.\r\n");
            xil_printf("Current generation length: %u tokens.\r\n", max_tokens);
            continue;
        }
        if (starts_with_ci(line, "len ")) {
            max_tokens = parse_len(line + 4);
            xil_printf("Generation length set to %u tokens.\r\n", max_tokens);
            continue;
        }
        if (line[0] == '\0') {
            continue;
        }

        gpt_perf_reset();
        copy_reply(g_gpt_perf.prompt, sizeof(g_gpt_perf.prompt), line);

        char raw[GPT_APP_MAX_NEW_TOKENS + 1];
        char clean[GPT_APP_MAX_NEW_TOKENS + 1];

        xil_printf("Generating with exported tiny GPT weights...\r\n");
        XTime total0;
        XTime total1;
        XTime_GetTime(&total0);
        int produced = gpt_generate(line, max_tokens, raw, sizeof(raw));
        XTime_GetTime(&total1);
        if (produced < 0) {
            produced = 0;
        }

        unsigned int clean_len = clean_completion(raw, clean, sizeof(clean));
        if (clean_len == 0U) {
            copy_reply(clean, sizeof(clean), raw);
            clean_len = (unsigned int)strlen(clean);
        }

        copy_reply(g_gpt_perf.generated, sizeof(g_gpt_perf.generated), clean);
        g_gpt_perf.token_count = (unsigned int)produced;
        if (g_gpt_perf.token_count > GPT_APP_MAX_NEW_TOKENS) {
            g_gpt_perf.token_count = GPT_APP_MAX_NEW_TOKENS;
        }
        g_gpt_perf.total_cycles = (unsigned long long)(total1 - total0);
        g_gpt_perf.fallback_used = 0U;

        unsigned long long avg = 0ULL;
        if (g_gpt_perf.token_count != 0U) {
            avg = g_gpt_perf.total_cycles / g_gpt_perf.token_count;
        }
        for (unsigned int i = 0U; i < g_gpt_perf.token_count; ++i) {
            g_gpt_perf.token_cycles[i] = avg;
        }

        xil_printf("GPT: %s\r\n", g_gpt_perf.generated);
    }
}
