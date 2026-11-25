#include "uart_io.h"

#include "sleep.h"
#include "xil_printf.h"
#include "xuartps_hw.h"
#include <string.h>

void uart_puts(const char *s) {
    while (*s != '\0') {
        XUartPs_SendByte(STDOUT_BASEADDRESS, (u8)*s++);
    }
}

void uart_readline(char *buf, size_t max_len) {
    size_t n = 0;
    if (max_len == 0) {
        return;
    }
    while (n + 1 < max_len) {
        char c = (char)XUartPs_RecvByte(STDIN_BASEADDRESS);
        if (c == '\r' || c == '\n') {
            xil_printf("\r\n");
            break;
        }
        if ((c == '\b' || c == 0x7f) && n > 0) {
            n--;
            xil_printf("\b \b");
            continue;
        }
        buf[n++] = c;
        xil_printf("%c", c);
    }
    buf[n] = '\0';
}

void uart_readline_reprompt(char *buf, size_t max_len, const char *prompt) {
    size_t n = 0;
    unsigned int idle_ticks = 0U;
    if (max_len == 0) {
        return;
    }
    while (n + 1 < max_len) {
        if (!XUartPs_IsReceiveData(STDIN_BASEADDRESS)) {
            usleep(10000U);
            idle_ticks++;
            if (idle_ticks >= 500U) {
                xil_printf("\r\n%s", prompt);
                idle_ticks = 0U;
            }
            continue;
        }
        idle_ticks = 0U;
        char c = (char)XUartPs_RecvByte(STDIN_BASEADDRESS);
        if (c == '\r' || c == '\n') {
            xil_printf("\r\n");
            break;
        }
        if ((c == '\b' || c == 0x7f) && n > 0) {
            n--;
            xil_printf("\b \b");
            continue;
        }
        buf[n++] = c;
        xil_printf("%c", c);
    }
    buf[n] = '\0';
}

int str_starts_with(const char *s, const char *prefix) {
    return strncmp(s, prefix, strlen(prefix)) == 0;
}
