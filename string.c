#include <string.h>

int32_t strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

int32_t strncmp(const char *s1, const char *s2, uint32_t n) {
    while (n--) {
        if (*s1 != *s2)
            return (unsigned char)*s1 - (unsigned char)*s2;
        if (*s1 == '\0')  // Kedua karakter pasti sama dan '\0'
            return 0;
        s1++;
        s2++;
    }
    return 0;
}
