#include "menu.h"

#include "xil_printf.h"
#include "xuartps.h"

static void uart0_raw_puts(const char *s) {
    volatile unsigned int *uart0_fifo = (volatile unsigned int *)0xFF000030U;
    while (*s != '\0') {
        *uart0_fifo = (unsigned int)(unsigned char)*s++;
    }
}

static void uart0_init_115200(void) {
    static XUartPs uart0;
    XUartPs_Config *cfg = XUartPs_LookupConfig(XPAR_XUARTPS_0_DEVICE_ID);
    if (cfg != 0) {
        XUartPs_CfgInitialize(&uart0, cfg, cfg->BaseAddress);
        XUartPs_SetBaudRate(&uart0, 115200);
    }
}

int main(void) {
    uart0_raw_puts("APP START\r\n");
    uart0_init_115200();
    uart0_raw_puts("UART INIT DONE\r\n");
    xil_printf("LeNet + tiny GPT FPGA demo\r\n");
    menu_run();
    return 0;
}
