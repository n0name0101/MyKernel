#ifndef __CPUID_H
#define __CPUID_H

#include <types.h>

static inline void enable_sse2(void) {
    uint32_t cr0, cr4;

    // Baca nilai CR0
    __asm__ volatile (
        "mov %%cr0, %0"
        : "=r" (cr0)
    );
    // Bersihkan TS (Task Switched) sehingga FPU/SSE dapat digunakan
    cr0 &= ~(1 << 3); /* Pada beberapa arsitektur, bit TS berada di posisi 3 */
    __asm__ volatile (
        "mov %0, %%cr0"
        :
        : "r" (cr0)
    );

    // Baca nilai CR4
    __asm__ volatile (
        "mov %%cr4, %0"
        : "=r" (cr4)
    );
    // Set bit OSFXSR (bit 9) dan OSXMMEXCPT (bit 10) untuk mendukung instruksi SSE
    cr4 |= (1 << 9) | (1 << 10);
    __asm__ volatile (
        "mov %0, %%cr4"
        :
        : "r" (cr4)
    );
}

// Declaration
void cpuid(int info[4], int leaf, int subleaf);
void detect_cpu_features(void);

#endif            //__CPUID_H
