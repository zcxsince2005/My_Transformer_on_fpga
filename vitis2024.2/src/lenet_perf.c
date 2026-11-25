#include "lenet_perf.h"

#include "xil_printf.h"
#include "xparameters.h"
#include "xtime_l.h"

#define GPU_KERAS_REPEAT 1000U
#define GPU_KERAS_BATCH1_AVG_MS_STR "1.381876"
#define FPGA_HLS_EST_MS_STR "0.125800"
#define SPEEDUP_STR "10.98"
#define PERF_TEST_LABEL 7U
#define PERF_TEST_PRED 7U

LenetPerf g_lenet_perf = {0U, 0U, 0xffffffffU, 0ULL, 0U, 0U, 0U, 0U, 0U, 0ULL};

static void print_cycles_as_ms(unsigned long long cycles)
{
    unsigned long long us = (cycles * 1000000ULL) / COUNTS_PER_SECOND;
    xil_printf("%llu.%03llu ms", us / 1000ULL, us % 1000ULL);
}

void lenet_perf_print(void) {
    xil_printf("========================================\r\n");
    xil_printf("Keras/GPU batch=1 single-image forward inference benchmark\r\n");
    xil_printf("repeat = %u\r\n", GPU_KERAS_REPEAT);
    xil_printf("average time = %s ms / image\r\n", GPU_KERAS_BATCH1_AVG_MS_STR);
    xil_printf("test label = %u\r\n", PERF_TEST_LABEL);
    xil_printf("pred = %u\r\n", PERF_TEST_PRED);
    xil_printf("Data note: GPU/Keras numbers are offline Python measurements, not GPU tests on board.\r\n");
    xil_printf("========================================\r\n");
    xil_printf("Single-image forward inference time comparison\r\n");
    xil_printf("FPGA HLS estimated single-image time: %s ms\r\n", FPGA_HLS_EST_MS_STR);
    xil_printf("GPU/Keras batch=1 single-image time: %s ms\r\n", GPU_KERAS_BATCH1_AVG_MS_STR);
    xil_printf("FPGA vs GPU batch=1 speedup: %s x\r\n", SPEEDUP_STR);

    if (g_lenet_perf.runs == 0U) {
        xil_printf("FPGA board measured cycles: not available yet. Run option 1 first.\r\n");
        xil_printf("========================================\r\n");
        return;
    }

    xil_printf("FPGA board measured runs: %u images\r\n", g_lenet_perf.runs);
    xil_printf("Last image: %u, label=%u, pred=%u\r\n",
               g_lenet_perf.last_image, g_lenet_perf.last_label, g_lenet_perf.last_prediction);
    xil_printf("Last single-image cycles: %u, ", g_lenet_perf.last_cycles);
    print_cycles_as_ms((unsigned long long)g_lenet_perf.last_cycles);
    xil_printf("\r\n");
    xil_printf("Best single-image cycles: %u, ", g_lenet_perf.best_cycles);
    print_cycles_as_ms((unsigned long long)g_lenet_perf.best_cycles);
    xil_printf("\r\n");
    xil_printf("Average single-image cycles: %llu, ", g_lenet_perf.total_cycles / g_lenet_perf.runs);
    print_cycles_as_ms(g_lenet_perf.total_cycles / g_lenet_perf.runs);
    xil_printf("\r\n");
    if (g_lenet_perf.batch_runs != 0U) {
        xil_printf("Last batch accuracy: %u / 10\r\n", g_lenet_perf.last_batch_correct);
        xil_printf("Last batch total cycles: %llu, ", g_lenet_perf.last_batch_cycles);
        print_cycles_as_ms(g_lenet_perf.last_batch_cycles);
        xil_printf("\r\n");
    }
    xil_printf("========================================\r\n");
}
