#ifndef _LINUX_STRING_H_
#define _LINUX_STRING_H_

#include<types.h>

static inline uint32_t strlen(const char * s)
{
	register int32_t __res __asm__("cx");
	__asm__("cld\n\t"
		"repne\n\t"
		"scasb\n\t"
		"notl %0\n\t"
		"decl %0"
		:"=c" (__res):"D" (s),"a" (0),"0" (0xffffffff):"cc");
	return __res;
}

static inline char *strncpy(char *dest, const char *src, uint32_t n) {
    uint32_t i;

    // Salin karakter satu per satu hingga n atau sampai null terminator
    for (i = 0; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];

    // Jika src lebih pendek dari n, isi sisanya dengan null
    for (; i < n; i++)
        dest[i] = '\0';

    return dest;
}

/* isdigit: Mengembalikan non-zero jika karakter c merupakan digit ('0' .. '9'), 0 jika bukan */
static inline int isdigit(int c) {
    return (c >= '0' && c <= '9');
}

int32_t strcmp(const char *s1, const char *s2);
int32_t strncmp(const char *s1, const char *s2, uint32_t n);

#endif
