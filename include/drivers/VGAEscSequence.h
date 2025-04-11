#ifndef __VGAESCSEQUENCE_H
#define __VGAESCSEQUENCE_H

#include <stdio.h>
#include <string.h>
#include <utils.h>

// Static Inline Function Declaration
static inline void process_sgr_color(VGA *vga_pointer, const char *seq);
static inline void CSIProcessing(VGA *vga_pointer,char *CSIsequence);

typedef enum {
    STATE_NORMAL,    // Tidak dalam escape sequence
    STATE_ESC,       // Sudah terdeteksi ESC (0x1b)
    STATE_CSI,       // Dalam CSI (ESC + '[')
    STATE_OSC,       // Dalam OSC (ESC + ']')
    STATE_OSC_ESC,   // Dalam OSC, sudah terdeteksi ESC sebagai potensi awal ST
    STATE_ESC_CHARSET // Untuk menangani escape sequence seperti ESC(B
} State;

extern int32_t esc_index;
extern State state;

static inline int32_t filter_escape_char(VGA *vga_pointer, char input) {
    // Buffer untuk menyimpan escape sequence yang sedang dibangun
    static char esc_buffer[256] = {0};

    switch (state) {
    case STATE_NORMAL:
        if (input == 0x1b) { // Mendeteksi ESC
            state = STATE_ESC;
            esc_index = 0;  // Reset buffer
            esc_buffer[esc_index++] = input;
            return -1;
        }
        else {
            // Bukan bagian escape, langsung kembalikan karakter
            return input;
        }

    case STATE_ESC:
        esc_buffer[esc_index++] = input;
        // Setelah ESC, periksa karakter berikutnya untuk menentukan tipe sequence
        if (input == '[') {
            state = STATE_CSI;
            return -1;
        }
        else if (input == ']') {
            state = STATE_OSC;
            return -1;
        }
        else if (input == '(') {
            // Mulai escape sequence untuk pemilihan charset, misal ESC(...
            state = STATE_ESC_CHARSET;
            esc_index = 0; // Reset buffer khusus untuk charset
            return -1;
        }
        else {
            state = STATE_NORMAL;
            return input;
        }

    case STATE_ESC_CHARSET:
        // Escape sequence untuk charset biasanya hanya satu karakter (misal 'B')
        esc_buffer[esc_index++] = input;
        esc_buffer[esc_index] = '\0';
        // process_charset(console, esc_buffer);
        state = STATE_NORMAL;
        esc_index = 0;
        return -1;

    case STATE_CSI:
        esc_buffer[esc_index++] = input;
        // Dalam CSI, sequence berakhir jika karakter final berada di rentang 0x40 sampai 0x7E
        if (input >= 0x40 && input <= 0x7E) {
            //printf("\nDetected CSI: ");
            esc_buffer[esc_index] = '\0';
            CSIProcessing(vga_pointer, esc_buffer); // Proses CSI sequence
            state = STATE_NORMAL;
            esc_index = 0;
        }
        return -1;

    case STATE_OSC:
        esc_buffer[esc_index++] = input;
        // Dalam OSC, sequence berakhir jika menemukan BEL (0x07) atau ST (ESC + '\\')
        if (input == 0x07) {
            // printf("\nDetected OSC: ");
            esc_buffer[--esc_index] = '\0';
            // process_osc(console, esc_buffer + 2);
            state = STATE_NORMAL;
            esc_index = 0;
        }
        else if (input == 0x1b) {
            // Potensi awal dari ST, pindah ke state OSC_ESC
            state = STATE_OSC_ESC;
        }
        return -1;

    case STATE_OSC_ESC:
        esc_buffer[esc_index++] = input;
        if (input == '\\') {
            // printf("\nDetected OSC: ");
            // for (int i = 0; i < esc_index; i++) {
            //     putchar(esc_buffer[i]);
            // }
            // printf("\n");
            state = STATE_NORMAL;
            esc_index = 0;
        }
        else {
            state = STATE_OSC;
        }
        return -1;

    default:
        state = STATE_NORMAL;
        return input;
    }
}

static inline void CSIProcessing(VGA *vga_pointer, char *CSIsequence) {
    int32_t len = strlen(CSIsequence);
    char final = CSIsequence[len - 1];
    // Hapus karakter final dari sequence untuk parsing parameter
    CSIsequence[len - 1] = '\0';

    switch (final) {
        case 'm':  // Warna
            process_sgr_color(vga_pointer, CSIsequence + 2);
            //printf("Color configuration\n");
            break;
        default:
            //printf("Unknown CSI sequence: %s%c\n", CSIsequence, final);
            break;
    }
}

/*
 * Fungsi process_sgr_color
 *
 * Parameter:
 *   fg   : pointer ke variabel uint32_t untuk foreground (ARGB)
 *   bg   : pointer ke variabel uint32_t untuk background (ARGB)
 *   seq  : string parameter SGR, misalnya "31;44" atau "0" untuk reset
 *
 * Jika seq kosong atau NULL, fungsi akan mereset fg dan bg ke nilai default:
 *   foreground default: 0xFFFFFFFF (putih)
 *   background default: 0x00000000 (hitam)
 *
 * Untuk nilai SGR:
 *   0     : reset (default)
 *   30-37 : set foreground dengan kode warna standar
 *   40-47 : set background dengan kode warna standar
 *
 * Kode warna ARGB yang digunakan:
 *   Black   : 0xFF000000
 *   Red     : 0xFFFF0000
 *   Green   : 0xFF00FF00
 *   Yellow  : 0xFFFFFF00
 *   Blue    : 0xFF0000FF
 *   Magenta : 0xFFFF00FF
 *   Cyan    : 0xFF00FFFF
 *   White   : 0xFFFFFFFF
 */
static inline void process_sgr_color(VGA *vga_pointer, const char *seq) {
    /* Reset ke default jika seq NULL atau kosong */
    if (seq == NULL || seq[0] == '\0') {
        vga_pointer->foreground = 0xFFFFFFFF;  /* white */
        vga_pointer->background = 0x00000000;  /* black */
        return;
    }

    const char *p = seq;
    while (*p) {
        /* Lewati karakter non-digit (misalnya, spasi atau pemisah) */
        while (*p && !isdigit(*p) && *p != '-' && *p != '+')
            p++;
        if (!*p)
            break;

        /* Konversi token angka menggunakan atol() */
        long code = atoi(p);

        /* Lewati token yang sudah dikonversi:
         * Lanjutkan hingga menemukan karakter yang bukan digit atau tanda
         */
        while (*p && (isdigit(*p) || *p == '-' || *p == '+'))
            p++;
        if (*p == ';')
            p++;  /* Lewati pemisah */

        /* Proses kode SGR */
        if (code == 0) {
            vga_pointer->foreground = 0xFFFFFFFF;  /* Reset foreground: white */
            vga_pointer->background = 0x00000000;  /* Reset background: black */
            vga_pointer->bold = FALSE;
        }
        else if (code == 1) {
            vga_pointer->bold = TRUE;
        }
        /* Standard Foreground Colors (30–37) */
        else if (code >= 30 && code <= 37) {
            switch(code) {
                case 30: vga_pointer->foreground = 0xFF000000; break;  /* Black */
                case 31: vga_pointer->foreground = 0xFFFF0000; break;  /* Red */
                case 32: vga_pointer->foreground = 0xFF00FF00; break;  /* Green */
                case 33: vga_pointer->foreground = 0xFFFFFF00; break;  /* Yellow */
                case 34: vga_pointer->foreground = 0xFF0000FF; break;  /* Blue */
                case 35: vga_pointer->foreground = 0xFFFF00FF; break;  /* Magenta */
                case 36: vga_pointer->foreground = 0xFF00FFFF; break;  /* Cyan */
                case 37: vga_pointer->foreground = 0xFFFFFFFF; break;  /* White */
            }
        }
        /* Bright Foreground Colors (90–97) */
        else if (code >= 90 && code <= 97) {
            switch(code) {
                case 90: vga_pointer->foreground = 0xFF808080; break;  /* Bright Black (Gray) */
                case 91: vga_pointer->foreground = 0xFFFF5555; break;  /* Bright Red */
                case 92: vga_pointer->foreground = 0xFF55FF55; break;  /* Bright Green */
                case 93: vga_pointer->foreground = 0xFFFFFF55; break;  /* Bright Yellow */
                case 94: vga_pointer->foreground = 0xFF5555FF; break;  /* Bright Blue */
                case 95: vga_pointer->foreground = 0xFFFF55FF; break;  /* Bright Magenta */
                case 96: vga_pointer->foreground = 0xFF55FFFF; break;  /* Bright Cyan */
                case 97: vga_pointer->foreground = 0xFFFFFFFF; break;  /* Bright White */
            }
        }

        /* Standard Background Colors (40–47) */
        else if (code >= 40 && code <= 47) {
            switch(code) {
                case 40: vga_pointer->background = 0xFF000000; break;  /* Black */
                case 41: vga_pointer->background = 0xFFFF0000; break;  /* Red */
                case 42: vga_pointer->background = 0xFF00FF00; break;  /* Green */
                case 43: vga_pointer->background = 0xFFFFFF00; break;  /* Yellow */
                case 44: vga_pointer->background = 0xFF0000FF; break;  /* Blue */
                case 45: vga_pointer->background = 0xFFFF00FF; break;  /* Magenta */
                case 46: vga_pointer->background = 0xFF00FFFF; break;  /* Cyan */
                case 47: vga_pointer->background = 0xFFFFFFFF; break;  /* White */
            }
        }
        /* Bright Background Colors (100–107) */
        else if (code >= 100 && code <= 107) {
            switch(code) {
                case 100: vga_pointer->background = 0xFF808080; break;  /* Bright Black (Gray) */
                case 101: vga_pointer->background = 0xFFFF5555; break;  /* Bright Red */
                case 102: vga_pointer->background = 0xFF55FF55; break;  /* Bright Green */
                case 103: vga_pointer->background = 0xFFFFFF55; break;  /* Bright Yellow */
                case 104: vga_pointer->background = 0xFF5555FF; break;  /* Bright Blue */
                case 105: vga_pointer->background = 0xFFFF55FF; break;  /* Bright Magenta */
                case 106: vga_pointer->background = 0xFF55FFFF; break;  /* Bright Cyan */
                case 107: vga_pointer->background = 0xFFFFFFFF; break;  /* Bright White */
            }
        }
    }
}

#endif         //__VGAESCSEQUENCE_H
