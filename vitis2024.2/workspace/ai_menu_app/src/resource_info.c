#include "resource_info.h"

#include "gpt_config.h"
#include "gpt_hw_accel.h"
#include "xil_printf.h"

#define DEV_LUT 230400U
#define DEV_FF 460800U
#define DEV_BRAM_TILE 312U
#define DEV_BRAM18 624U
#define DEV_URAM 96U
#define DEV_DSP 1728U

#define LENET_LUT 77577U
#define LENET_FF 53450U
#define LENET_BRAM18 135U
#define LENET_URAM 0U
#define LENET_DSP 823U

#define GPT_LUT 67743U
#define GPT_FF 55903U
#define GPT_BRAM18 20U
#define GPT_URAM 1U
#define GPT_DSP 204U

#define FULL_LUT 117328U
#define FULL_FF 130452U
#define FULL_BRAM_TILE 61U
#define FULL_URAM 1U
#define FULL_DSP 1026U

static void print_pct(unsigned int used, unsigned int total)
{
    unsigned int x100 = (total == 0U) ? 0U : (used * 10000U) / total;
    xil_printf("%u.%02u%%", x100 / 100U, x100 % 100U);
}

void resource_info_print(void)
{
    xil_printf("========================================\r\n");
    xil_printf("FPGA Resource Utilization - HLS V4\r\n");
    xil_printf("Target: XCZU7EV-FFVC1156-2-I\r\n\r\n");

    xil_printf("Device Total:\r\n");
    xil_printf("LUT: %u\r\n", DEV_LUT);
    xil_printf("FF: %u\r\n", DEV_FF);
    xil_printf("BRAM Tile: %u\r\n", DEV_BRAM_TILE);
    xil_printf("BRAM_18K: %u\r\n", DEV_BRAM18);
    xil_printf("URAM: %u\r\n", DEV_URAM);
    xil_printf("DSP: %u\r\n\r\n", DEV_DSP);

    xil_printf("LeNet IP:\r\n");
    xil_printf("LUT: %u\r\n", LENET_LUT);
    xil_printf("FF: %u\r\n", LENET_FF);
    xil_printf("BRAM_18K: %u\r\n", LENET_BRAM18);
    xil_printf("BRAM Tile approx: %u\r\n", (LENET_BRAM18 + 1U) / 2U);
    xil_printf("URAM: %u\r\n", LENET_URAM);
    xil_printf("DSP: %u\r\n", LENET_DSP);
    xil_printf("Source: HLS lenet csynth report\r\n\r\n");

    xil_printf("GPT IP:\r\n");
    xil_printf("LUT: %u\r\n", GPT_LUT);
    xil_printf("FF: %u\r\n", GPT_FF);
    xil_printf("BRAM_18K: %u\r\n", GPT_BRAM18);
    xil_printf("BRAM Tile approx: %u\r\n", (GPT_BRAM18 + 1U) / 2U);
    xil_printf("URAM: %u\r\n", GPT_URAM);
    xil_printf("DSP: %u\r\n", GPT_DSP);
    xil_printf("Model: n_embd=%u, n_head=%u, n_layer=%u, mlp_hidden=%u\r\n",
               GPT_N_EMBD, GPT_N_HEAD, GPT_N_LAYER, GPT_MLP_HIDDEN);
    xil_printf("HLS parallel factor: %u\r\n", GPT_V4_HLS_LINEAR_PARALLEL_FACTOR);
    xil_printf("HLS matmul output parallel factor: %u\r\n", GPT_V4_HLS_MATMUL_OUT_PARALLEL_FACTOR);
    xil_printf("Source: HLS gpt_accel csynth report\r\n\r\n");

    xil_printf("Full Routed Design:\r\n");
    xil_printf("LUT: %u / %u = ", FULL_LUT, DEV_LUT);
    print_pct(FULL_LUT, DEV_LUT);
    xil_printf("\r\n");
    xil_printf("FF: %u / %u = ", FULL_FF, DEV_FF);
    print_pct(FULL_FF, DEV_FF);
    xil_printf("\r\n");
    xil_printf("BRAM Tile: %u / %u = ", FULL_BRAM_TILE, DEV_BRAM_TILE);
    print_pct(FULL_BRAM_TILE, DEV_BRAM_TILE);
    xil_printf("\r\n");
    xil_printf("URAM: %u / %u = ", FULL_URAM, DEV_URAM);
    print_pct(FULL_URAM, DEV_URAM);
    xil_printf("\r\n");
    xil_printf("DSP: %u / %u = ", FULL_DSP, DEV_DSP);
    print_pct(FULL_DSP, DEV_DSP);
    xil_printf("\r\n");
    xil_printf("Source: Vivado routed utilization report\r\n\r\n");

    xil_printf("Notes:\r\n");
    xil_printf("V4 returns to the 96-wide GPT model and keeps LINEAR_ROLLING_KV stable.\r\n");
    xil_printf("V4 reduces PS cache maintenance while keeping numerically aligned Linear HLS calls.\r\n");
    xil_printf("PL clock is set to 150 MHz to meet routed timing with the wider GPT matmul.\r\n");
}
