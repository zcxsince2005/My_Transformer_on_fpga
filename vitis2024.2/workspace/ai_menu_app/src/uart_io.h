#ifndef UART_IO_H
#define UART_IO_H

#include <stddef.h>

void uart_puts(const char *s);
void uart_readline(char *buf, size_t max_len);
void uart_readline_reprompt(char *buf, size_t max_len, const char *prompt);
int str_starts_with(const char *s, const char *prefix);

#endif
