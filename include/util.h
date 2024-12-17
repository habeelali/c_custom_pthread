#ifndef UTIL_H
#define UTIL_H

#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned long addr_t;

#define LOG_OUT(format, ...)                    \
    do                                          \
    {                                           \
        write_formatted(format, ##__VA_ARGS__); \
    } while (0)

void write_formatted(const char *format, ...);

void enable_interrupts();
void disable_interrupts();
addr_t mangle(addr_t addr);

#endif
