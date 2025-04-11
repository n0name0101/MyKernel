#include <utils.h>
#include <boolean.h>

extern bool g_has_sse2;

int32_t atoi(const char *str) {
    int32_t result = 0;
    int32_t sign = 1;

    // Lewati spasi dan whitespace lainnya
    while (*str == ' ' || *str == '\t' || *str == '\n' ||
           *str == '\v' || *str == '\f' || *str == '\r') {
        str++;
    }

    // Tangani tanda
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }

    // Proses konversi setiap karakter digit
    while (*str) {
        // Jika karakter bukan digit, hentikan parsing
        if (*str < '0' || *str > '9')
            break;
        result = result * 10 + (*str - '0');
        str++;
    }

    return sign * result;
}

void *memset(void *dest, uint8_t val, uint32_t len) {
    uint8_t *ptr = (uint8_t *)dest;

    if (g_has_sse2 && len >= 16) {
        uint32_t blocks = len / 16;
        uint32_t remainder = len % 16;

        // Siapkan nilai 16 byte yang berisi val
        uint8_t buffer[16];
        for (int i = 0; i < 16; i++) {
            buffer[i] = val;
        }

        asm volatile (
            "movdqu (%2), %%xmm0\n\t"    // Load 16-byte val ke xmm0
            "1:\n\t"
            "movdqu %%xmm0, (%0)\n\t"    // Store 16-byte ke ptr
            "add $16, %0\n\t"
            "dec %1\n\t"
            "jnz 1b\n\t"
            : "+r" (ptr), "+r" (blocks)
            : "r" (buffer)
            : "xmm0", "memory"
        );

        // Sisa byte (kurang dari 16)
        while (remainder--) {
            *ptr++ = val;
        }
    } else {
        // Fallback: byte-by-byte
        while (len-- > 0) {
            *ptr++ = val;
        }
    }

    return dest;
}


// void *memset(void *dest, uint8_t val, uint32_t len) {
//     unsigned char *ptr = (unsigned char *)dest;
//     while (len-- > 0)
//         *ptr++ = (unsigned char)val;
//     return dest;
// }

// void *memcpy(void *dest, const void *src, uint32_t len) {
//     asm volatile (
//         "cld\n\t"               // Clear direction flag
//         "rep movsb"            // Copy byte by byte
//         : "=D" (dest), "=S" (src), "=c" (len)
//         : "0" (dest), "1" (src), "2" (len)
//         : "memory"
//     );
//     return dest;
// }


void *memcpy(void *dest, const void *src, uint32_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    /* Jika SSE2 tersedia dan ukuran data minimal 16 byte */
    if (g_has_sse2 && n >= 16) {
        uint32_t blocks = n / 16;    /* Jumlah blok 16 byte */
        uint32_t remainder = n % 16; /* Sisa byte yang tidak kelipatan 16 */

        /*
         * Menggunakan inline assembly untuk menyalin 16 byte per iterasi.
         * - Menggunakan instruksi movdqu (move unaligned 128-bit data)
         * - Menambah pointer masing-masing dengan 16 setiap iterasi
         * - Menggunakan counter blocks sebagai loop counter
         */
        asm volatile (
            "1:\n\t"
            "movdqu    (%1), %%xmm0\n\t"  /* Load 16 byte dari [s] ke xmm0 */
            "movdqu    %%xmm0, (%0)\n\t"  /* Simpan 16 byte dari xmm0 ke [d] */
            "add       $16, %0\n\t"       /* d = d + 16 */
            "add       $16, %1\n\t"       /* s = s + 16 */
            "dec       %2\n\t"            /* Decrement block counter */
            "jnz       1b\n\t"            /* Ulangi jika belum habis */
            : "+r" (d), "+r" (s), "+r" (blocks)
            :
            : "xmm0", "memory"
        );

        /* Salin sisa byte yang tidak habis dibagi 16 */
        while (remainder--) {
            *d++ = *s++;
        }
    } else {
        /* Jika SSE2 tidak tersedia, lakukan penyalinan byte-per-byte */
        while (n--) {
            *d++ = *s++;
        }
    }
    return dest;
}
