#ifndef __UTILS_H
#define __UTILS_H

#include <types.h>

void *memset(void *dest, uint8_t val, uint32_t len);
void *memcpy(void *dest, const void *src, uint32_t len);
int32_t atoi(const char *str);

#endif            // __UTILS_H
