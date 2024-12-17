#include "util.h"
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <pthread.h>

extern sigset_t signal_mask;
/*
    This function is taken from glibc library
    https://github.com/bminor/glibc/blob/5ef608f36493c5d711418c5d31a7ebe710decc6e/sysdeps/unix/sysv/linux/x86_64/pointer_guard.h#L29
    It takes the register given and mangles it, to deal with
    memory protection randomizations, since we are placing
    the given address on the kernel. The mangling process
    sets a constant value at the start of the program and
    we use the piece of code below to mangle and unmangle it
    It uses a simple XOR (which is why we can mangle and unmangle
    with the same piece of code)
*/
addr_t mangle(addr_t address)
{
    addr_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
                 : "=g"(ret)
                 : "0"(address));
    return ret;
}

void enable_interrupts()
{
    sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
}

void disable_interrupts()
{
    sigprocmask(SIG_BLOCK, &signal_mask, NULL);
}

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to write a formatted string to standard output
void write_formatted(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    // Lock the mutex before entering the critical section
    pthread_mutex_lock(&mutex);

    // Iterate through the format string
    while (*format != '\0')
    {
        if (*format == '%')
        {
            fflush(fdopen(STDOUT_FILENO, "w"));
            format++; // Move past '%'
            // Handle format specifiers
            switch (*format)
            {
            case 'd':
            {
                int value = va_arg(args, int);
                char buffer[20]; // Assuming a reasonable buffer size
                int len = 0;
                while (value != 0)
                {
                    buffer[len++] = '0' + value % 10;
                    value /= 10;
                }
                // Handle the case where the value is 0
                if (len == 0)
                {
                    buffer[len++] = '0';
                }
                // Write the reversed buffer to standard output
                while (len > 0)
                {
                    write(STDOUT_FILENO, &buffer[--len], 1);
                }
                break;
            }
            case 's':
            {
                char *str = va_arg(args, char *);
                // Write the string to standard output
                while (*str != '\0')
                {
                    write(STDOUT_FILENO, str++, 1);
                }
                break;
            }
            case 'p': // Pointer
            {
                void *ptr = va_arg(args, void *);
                unsigned long addr = (unsigned long)ptr;
                char buffer[20];
                int len = 0;
                buffer[len++] = '0';
                buffer[len++] = 'x';
                if (addr == 0)
                {
                    buffer[len++] = '0';
                }
                else
                {
                    for (int i = (sizeof(addr) * 2) - 5; i >= 0; --i)
                    {
                        int nibble = (addr >> (i * 4)) & 0xF;
                        buffer[len++] = (nibble < 10) ? '0' + nibble : 'a' + nibble - 10;
                    }
                }
                for (int i = 0; i < len; i++)
                {
                    write(STDOUT_FILENO, &buffer[i], 1);
                }
                break;
            }
            case 'f': // Float
            {
                double value = va_arg(args, double);
                int int_part = (int)value;
                double frac_part = value - int_part;
                char buffer[20];
                int len = 0;

                // Handle integer part
                if (int_part == 0)
                {
                    buffer[len++] = '0';
                }
                else
                {
                    while (int_part != 0)
                    {
                        buffer[len++] = '0' + int_part % 10;
                        int_part /= 10;
                    }
                }

                // Write the integer part in reverse order
                for (int i = len - 1; i >= 0; i--)
                {
                    write(STDOUT_FILENO, &buffer[i], 1);
                }

                // Handle fractional part
                write(STDOUT_FILENO, ".", 1);
                for (int i = 0; i < 2; i++)
                {
                    frac_part *= 10;
                    int frac_digit = (int)frac_part;
                    char digit = '0' + frac_digit;
                    write(STDOUT_FILENO, &digit, 1);
                    frac_part -= frac_digit;
                }
                break;
            }

            // Handle other characters if needed
            default:
                write(STDOUT_FILENO, format, 1);
            }
        }
        else
        {
            // Just write non-format characters
            write(STDOUT_FILENO, format, 1);
        }

        fflush(stdout);
        format++; // Move to the next character in the format string
    }

    // Unlock the mutex before exiting the function
    pthread_mutex_unlock(&mutex);

    va_end(args);
}
