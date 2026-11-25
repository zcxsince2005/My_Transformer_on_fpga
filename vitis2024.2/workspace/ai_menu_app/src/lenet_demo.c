#include "lenet_demo.h"

#include "lenet_perf.h"
#include "mnist_10_test_imgs.h"
#include "platform_config.h"
#include "xil_cache.h"
#include "xil_io.h"
#include "xil_printf.h"
#include "xil_types.h"
#include "xmyproject_axi_hw.h"
#include "xtime_l.h"

#define INPUT_WORDS 392U
#define OUTPUT_WORDS 5U
#define IMAGE_COUNT 10U
#define IP_TIMEOUT 100000000U
#define LENET_INPUT_DDR_ADDR 0x10000000U
#define LENET_OUTPUT_DDR_ADDR 0x10001000U

static volatile u32 *const in_buf = (volatile u32 *)(UINTPTR)LENET_INPUT_DDR_ADDR;
static volatile u32 *const out_buf = (volatile u32 *)(UINTPTR)LENET_OUTPUT_DDR_ADDR;

static void write_addr64(u32 base, u32 reg_low_offset, u64 addr)
{
    Xil_Out32(base + reg_low_offset, (u32)(addr & 0xffffffffULL));
    Xil_Out32(base + reg_low_offset + 4U, (u32)((addr >> 32) & 0xffffffffULL));
}

static void lenet_start(u32 base)
{
    u32 ctrl = Xil_In32(base + XMYPROJECT_AXI_CONTROL_ADDR_AP_CTRL);
    Xil_Out32(base + XMYPROJECT_AXI_CONTROL_ADDR_AP_CTRL, ctrl | 0x01U);
}

static int lenet_wait_done(u32 base, u32 timeout)
{
    for (u32 i = 0; i < timeout; ++i) {
        if ((Xil_In32(base + XMYPROJECT_AXI_CONTROL_ADDR_AP_CTRL) & 0x2U) != 0U) {
            return 0;
        }
    }
    return -1;
}

static void unpack_scores(short scores[10])
{
    for (u32 i = 0; i < OUTPUT_WORDS; ++i) {
        u32 w = (u32)out_buf[i];
        scores[2U * i] = (short)(w & 0xffffU);
        scores[2U * i + 1U] = (short)((w >> 16) & 0xffffU);
    }
}

static int argmax_scores(const short scores[10])
{
    int best_idx = 0;
    short best_val = scores[0];
    for (int i = 1; i < 10; ++i) {
        if (scores[i] > best_val) {
            best_val = scores[i];
            best_idx = i;
        }
    }
    return best_idx;
}

static int lenet_run_one(unsigned int img_idx, short scores[10], unsigned long long *cycles)
{
    for (u32 i = 0; i < INPUT_WORDS; ++i) {
        in_buf[i] = test_imgs[img_idx][i];
    }
    for (u32 i = 0; i < OUTPUT_WORDS; ++i) {
        out_buf[i] = 0U;
    }

    Xil_DCacheFlushRange((UINTPTR)in_buf, INPUT_WORDS * (u32)sizeof(u32));
    Xil_DCacheFlushRange((UINTPTR)out_buf, OUTPUT_WORDS * (u32)sizeof(u32));

    write_addr64(LENET_BASEADDR, XMYPROJECT_AXI_CONTROL_ADDR_IN_PTR_DATA, (u64)(UINTPTR)in_buf);
    write_addr64(LENET_BASEADDR, XMYPROJECT_AXI_CONTROL_ADDR_OUT_PTR_DATA, (u64)(UINTPTR)out_buf);

    XTime t0;
    XTime t1;
    XTime_GetTime(&t0);
    lenet_start(LENET_BASEADDR);
    int ret = lenet_wait_done(LENET_BASEADDR, IP_TIMEOUT);
    XTime_GetTime(&t1);
    *cycles = (unsigned long long)(t1 - t0);

    if (ret != 0) {
        return -1;
    }

    Xil_DCacheInvalidateRange((UINTPTR)out_buf, OUTPUT_WORDS * (u32)sizeof(u32));
    unpack_scores(scores);
    return 0;
}

void lenet_run_demo(void)
{
    int correct = 0;
    unsigned long long batch_cycles = 0ULL;

    xil_printf("10-image digit recognition batch test:\r\n");
    for (unsigned int img_idx = 0U; img_idx < IMAGE_COUNT; ++img_idx) {
        short scores[10];
        unsigned long long cycles = 0ULL;
        int ret = lenet_run_one(img_idx, scores, &cycles);
        batch_cycles += cycles;

        if (ret != 0) {
            xil_printf("img %u, label=%d, pred=TIMEOUT\r\n", img_idx, test_labels[img_idx]);
            continue;
        }

        int pred = argmax_scores(scores);
        if (pred == test_labels[img_idx]) {
            ++correct;
        }

        g_lenet_perf.runs++;
        g_lenet_perf.last_cycles = (unsigned int)cycles;
        g_lenet_perf.total_cycles += cycles;
        if (cycles < (unsigned long long)g_lenet_perf.best_cycles) {
            g_lenet_perf.best_cycles = (unsigned int)cycles;
        }
        g_lenet_perf.last_prediction = (unsigned int)pred;
        g_lenet_perf.last_label = (unsigned int)test_labels[img_idx];
        g_lenet_perf.last_image = img_idx;

        xil_printf("img %u, label=%d, pred=%d\r\n", img_idx, test_labels[img_idx], pred);
        if (pred != test_labels[img_idx]) {
            xil_printf("scores: ");
            for (int i = 0; i < 10; ++i) {
                xil_printf("%d ", scores[i]);
            }
            xil_printf("\r\n");
        }
    }

    g_lenet_perf.batch_runs++;
    g_lenet_perf.last_batch_correct = (unsigned int)correct;
    g_lenet_perf.last_batch_cycles = batch_cycles;

    xil_printf("\r\naccuracy = %d / 10\r\n", correct);
    xil_printf("========= test end =========\r\n");
}
