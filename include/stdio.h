#ifndef __STDIO_H
#define __STDIO_H

#include <types.h>
#include <drivers/VGAColorSeq.h>

#define PRINTF_DEBUG(fmt, ...) \
    printf(BRIGHT_BLACK "[DBG]" RESET WHITE " " fmt RESET "\n", ##__VA_ARGS__)

void printf(const char *format, ...);
int32_t my_isprint(int32_t c);
int writeVGABuffer(char _data);
void cleanScreen();

#endif            // __STDIO_H
