#include <drivers/cpuid.h>
#include <boolean.h>

// Global flags untuk dukungan fitur CPU
bool g_has_avx2 = FALSE;
bool g_has_sse2 = FALSE;

// Fungsi wrapper CPUID: isi info[0]=EAX, info[1]=EBX, info[2]=ECX, info[3]=EDX
void cpuid(int info[4], int leaf, int subleaf) {
    asm __volatile__("cpuid"
                     : "=a"(info[0]), "=b"(info[1]), "=c"(info[2]), "=d"(info[3])
                     : "a"(leaf), "c"(subleaf));
}

// Fungsi untuk membaca XCR0 (required untuk AVX)
// Menggunakan instruksi XGETBV (kode biner agar tak ada warning terkait nama instruksi)
uint64_t xgetbv(uint32_t index) {
    uint32_t eax, edx;
    asm __volatile__(".byte 0x0f, 0x01, 0xd0"
                     : "=a"(eax), "=d"(edx)
                     : "c"(index));
    return ((uint64_t)edx << 32) | eax;
}

// Fungsi untuk mendeteksi dukungan SSE2 dan AVX2 secara runtime
void detect_cpu_features(void) {
   g_has_sse2 = FALSE;
   g_has_avx2 = FALSE;

   int info[4];
   // Leaf 1 untuk mendeteksi SSE2 dan AVX (dari ECX/EDX)
   cpuid(info, 1, 0);
   bool sse2_supported = (info[3] & (1 << 26)) != 0;  // EDX bit 26
   bool avx_supported = (info[2] & (1 << 28)) != 0;     // ECX bit 28
   // Untuk menggunakan AVX, OS harus mendukung XSAVE dan XGETBV
   bool xsave_supported = (info[2] & (1 << 27)) != 0;   // ECX bit 27

   bool os_avx_enabled = FALSE;
   if (avx_supported && xsave_supported) {
     uint64_t xcr0 = xgetbv(0);
     // Pastikan XCR0[1:0] (XMM dan YMM) aktif
     os_avx_enabled = ((xcr0 & 0x6) == 0x6);
   }
   bool avx_enabled = avx_supported && xsave_supported && os_avx_enabled;

   // Jika AVX aktif, kita bisa cek AVX2 (leaf 7, subleaf 0)
   bool avx2_supported = FALSE;
   if (avx_enabled) {
     cpuid(info, 7, 0);
     avx2_supported = (info[1] & (1 << 5)) != 0;  // EBX bit 5
   }

   // Simpan hasil deteksi global
   if (sse2_supported)    enable_sse2();
   g_has_sse2 = sse2_supported;
   g_has_avx2 = avx2_supported;
}
