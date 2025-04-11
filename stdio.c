#include <types.h>
#include <stdio.h>
#include <string.h>
#include <boolean.h>
#include <drivers/VGA.h>

int32_t my_isprint(int32_t c) {
    // Karakter printable berada di antara 0x20 (spasi) dan 0x7E (tilde)
    return (c >= 0x20 && c <= 0x7E);
}

static inline void printf_hex(const uint32_t num, int32_t min_digits) {
   // Fungsi rekursif untuk mencetak digit heksadesimal dari kiri ke kanan.
   void print_hex_recursive(uint32_t num) {
      if (num >= 16)
         print_hex_recursive(num >> 4);            // divisor >> 4 = divisor / 16
      write_font("0123456789ABCDEF"[num % 16]);
   }

   uint32_t numtmp = num;
   // Hitung jumlah digit yang dihasilkan.
   int digit_count = 0;
   if (num == 0)
      digit_count = 1;
   else
      digit_count = (32 - __builtin_clz(num) + 3) >> 2;         // Menghitung jumlah digit hexadecimal menggunakan __builtin_clz untuk efisiensi.  x>>2 = x/4

   // Padding: jika jumlah digit kurang dari min_digits, tambahkan nol di kiri.
   int pad = (min_digits > digit_count) ? (min_digits - digit_count) : 0;
   // Selain itu, jika jumlah digit (setelah padding) ganjil, tambahkan satu nol ekstra.
   if (((digit_count + pad) & 1) == 1)
      pad++;
   for (int i = 0; i < pad; i++)
      write_font('0');

   // Dengan Rekursif
   print_hex_recursive(num);

   // Dengan Iteration
   // Hitung nilai divisor: 16^(digit_count-1)
   // uint32_t divisor = 1;
   // for (int i = 1; i < digit_count; i++)
   //    divisor <<= 4;      // divisor << 4 = divisor * 16
   //
   // // Loop untuk mencetak tiap digit mulai dari yang paling kiri
   // for (int i = 0; i < digit_count; i++) {
   //    uint32_t digit = numtmp / divisor;
   //
   //    write_font("0123456789ABCDEF"[digit]);
   //    numtmp %= divisor;
   //    divisor >>= 4;      // divisor >> 4 = divisor / 16
   // }
}

static inline void printf_int(const int32_t num, const uint8_t base, const bool sign, int32_t min_digits) {

   void print_int_recursive(uint32_t num, uint8_t base) {
      if (num >= base)
         print_int_recursive(num / base, base);
      write_font("0123456789ABCDEF"[num % base]);
   }

   bool negative = FALSE;
   uint32_t n;
   // Jika format signed dan angka negatif, cetak '-' terlebih dahulu.
   if (sign && num < 0) {
     negative = TRUE;
     write_font('-');
     n = -num;
   } else
     n = num;

   // Hitung jumlah digit untuk angka (tanpa tanda negatif)
   int digit_count = 0;
   uint32_t tmp = n;
   do {
      digit_count++;
      tmp /= base;
   } while(tmp != 0);

   // Tambahkan padding nol (di sebelah kiri angka) jika diperlukan.
   int pad = (min_digits > digit_count) ? (min_digits - digit_count) : 0;
   for (int i = 0; i < pad; i++)
      write_font('0');

   // Cetak angka secara rekursif.
   print_int_recursive(n, base);
}

static inline void printf_bin(const uint32_t num, int32_t min_digits) {
   void print_bin_recursive(uint32_t num) {
      if (num >= 2)
         print_bin_recursive(num / 2);
      write_font("01"[num % 2]);
   }

   // Hitung jumlah digit biner.
   int digit_count = 0;
   uint32_t tmp = num;
   do {
      digit_count++;
      tmp /= 2;
   } while(tmp != 0);

   // Tambahkan padding nol untuk mencapai minimal digit yang diminta.
   int pad = (min_digits > digit_count) ? (min_digits - digit_count) : 0;
   for (int i = 0; i < pad; i++)
      write_font('0');

   // Cetak digit biner secara rekursif.
   print_bin_recursive(num);
}

// Fungsi printf sederhana dengan dukungan format: %d, %u, %x, %b, %s, %c, dan %%
void printf(const char *fmt, ...) {
    // Gunakan pointer ke argumen yang disusun di stack (metode non-va_args)
    uint32_t *arg_ptr = (uint32_t *)&fmt;
    arg_ptr++;  // Lewati parameter fmt

    int state = 0;  // 0: mode normal, 1: setelah menemukan '%'
    char *s = 0;
    bool alternate_form = FALSE, left_adjust = FALSE;
    int32_t precision = 0;
    int32_t field_width = 0;

    for (uint32_t i = 0; fmt[i] != '\0'; i++) {
        char c = fmt[i];
        if (state == 0) {
            if (c == '%')
                state = 1;
            else
                write_font(c);
        } else if (state == 1) {
            // Reset semua flag dan parameter
            alternate_form = FALSE;
            left_adjust    = FALSE;
            precision = 0;
            field_width = 0;

            // Proses flag format
            while (TRUE) {
                if (c == '#') {
                    alternate_form = TRUE;
                    c = fmt[++i];
                    continue;
                } else if (c == '-') {
                    left_adjust = TRUE;
                    c = fmt[++i];
                    continue;
                }
                break;
            }

            // Proses field width
            while (c >= '0' && c <= '9') {
                field_width = field_width * 10 + (c - '0');
                c = fmt[++i];
            }

            // Proses precision jika ada
            if (c == '.') {
                c = fmt[++i];
                while (c >= '0' && c <= '9') {
                    precision = precision * 10 + (c - '0');
                    c = fmt[++i];
                }
            }

            // Tangani specifier konversi
            switch (c) {
            case 'd': {
                int32_t number = *(int32_t *)arg_ptr;
                arg_ptr++;
                // Panggil fungsi cetak desimal (signed)
                printf_int(number, 10, TRUE, precision);
                break;
            }
            case 'u': {
                uint32_t number = *(uint32_t *)arg_ptr;
                arg_ptr++;
                // Panggil fungsi cetak desimal (unsigned)
                printf_int(number, 10, FALSE, precision);
                break;
            }
            case 'x': {
                uint32_t number = *(uint32_t *)arg_ptr;
                arg_ptr++;
                if (alternate_form) {
                    write_font('0');
                    write_font('x');
                }
                printf_hex(number, precision);
                break;
            }
            case 'b': {
                uint32_t number = *(uint32_t *)arg_ptr;
                arg_ptr++;
                if (alternate_form) {
                    write_font('0');
                    write_font('b');
                }
                printf_bin(number, precision);
                break;
            }
            case 's': {
                s = *(char **)arg_ptr;
                arg_ptr++;
                if (!s)
                    s = "(null)";
                int32_t len = strlen(s);
                if (field_width > len && !left_adjust) {
                    for (int32_t j = 0; j < field_width - len; j++)
                        write_font(' ');
                }
                while (*s)
                    write_font(*s++);
                if (field_width > len && left_adjust) {
                    for (int32_t j = 0; j < field_width - len; j++)
                        write_font(' ');
                }
                break;
            }
            case 'c': {
                write_font(*(uint32_t *)arg_ptr);
                arg_ptr++;
                break;
            }
            case '%': {
                write_font('%');
                break;
            }
            default: {
                // Jika specifier tidak didukung, tampilkan literal.
                write_font('%');
                write_font(c);
                break;
            }
            }
            state = 0;
        }
    }
}

// Print a single character to stdout
void putchar(char c) {
    printf("%c", c);
}

// Print a string with ending newline to stdout
void puts(char *s) {
    printf("%s\n", s);
}
