#include "menu.h"

#include "gpt_dialog.h"
#include "gpt_perf.h"
#include "lenet_demo.h"
#include "lenet_perf.h"
#include "resource_info.h"
#include "uart_io.h"
#include "xil_printf.h"
#include <string.h>

void menu_run(void) {
    char line[32];
    while (1) {
        xil_printf("\r\n");
        xil_printf("========================================\r\n");
        xil_printf("Zynq MPSoC AI Demo HLS V4\r\n");
        xil_printf("Target: XCZU7EV-FFVC1156-2-I\r\n\r\n");
        xil_printf("Menu:\r\n");
        xil_printf("1. Run digit recognition demo\r\n");
        xil_printf("2. Print digit recognition performance\r\n");
        xil_printf("3. Enter tiny GPT dialogue\r\n");
        xil_printf("4. Print last tiny GPT generation performance\r\n");
        xil_printf("5. Print FPGA resource utilization\r\n");
        xil_printf("# Please select: ");
        uart_readline(line, sizeof(line));
        if (strcmp(line, "1") == 0) {
            lenet_run_demo();
        } else if (strcmp(line, "2") == 0) {
            lenet_perf_print();
        } else if (strcmp(line, "3") == 0) {
            gpt_dialog_run();
        } else if (strcmp(line, "4") == 0) {
            gpt_perf_print();
        } else if (strcmp(line, "5") == 0) {
            resource_info_print();
        } else if (strcmp(line, "help") == 0) {
            xil_printf("Enter 1, 2, 3, 4, or 5. In tiny GPT mode, type exit to return.\r\n");
        } else if (strcmp(line, "exit") == 0) {
            xil_printf("Exit ignored at main menu.\r\n");
        } else {
            xil_printf("Invalid command\r\n");
        }
    }
}
