#include <drivers/VGA.h>
#include <drivers/VGAEscSequence.h>
#include <drivers/VGAColorSeq.h>
#include <drivers/TimerPIT.h>
#include <stdio.h>
#include <ioport.h>
#include <utils.h>
#include <types.h>
#include <boolean.h>
#include <font10x18.h>

extern bool g_has_sse2;
extern uint32_t* vbe_physical_base_pointer;
extern uint16_t x_resolution;          // Width
extern uint16_t y_resolution;          // Height

VGA KernelVGA = {
   .window_width = 0,
   .window_height = 0,
   .totalPixel = 0,
   .leftrightmarginpiksel = 1,
   .cursor_x = 0,
   .cursor_y = 0,
   .bold = FALSE,
   .foreground = 0xFFFFFFFF,
   .background = 0x00000000,
   .cursor_set = FALSE,
   .frameBufPointer = NULL
};
VGA font_layer_attribute = {0};
int32_t esc_index = 0;
State state = STATE_NORMAL;


// Declaration
void set_cursor(void *);

void initVGA() {
   // *Initialize the global Variable           // Jika tidak mesin fisik ga bisa jalan, tapi virtual machine jalan wkkwkwkw
   KernelVGA = (VGA){
      .window_width = x_resolution,
      .window_height = y_resolution,
      .totalPixel = x_resolution*y_resolution,
      .leftrightmarginpiksel = 1,
      .cursor_x = 0,
      .cursor_y = 0,
      .bold = FALSE,
      .foreground = 0xFFFFFFFF,
      .background = 0x00000000,
      .cursor_set = FALSE,
      .frameBufPointer = vbe_physical_base_pointer
   };
   esc_index = 0;
   state = STATE_NORMAL;


   move_cursor(0, 0);

   clear_screen();

   printf(CYAN  "[ >> ] Initializing VGA Mode\n" RESET);
   printf("        Framebuffer : " YELLOW "0x%.8x\n" RESET, KernelVGA.frameBufPointer);
   printf("        Resolution  : " YELLOW "%dx%d\n\n" RESET, x_resolution, y_resolution);
}

static inline void draw_normal_glyph_row(VGA VGAAttr, uint32_t *pixel, uint32_t bits) {
    for (uint32_t col = 0; col < FONT_WIDTH; col++) {
        *pixel = (bits & 0x80000000) ? VGAAttr.foreground : VGAAttr.background;
        bits <<= 1;
        pixel++;
    }
}

static inline void draw_bold_glyph_row(VGA VGAAttr, uint32_t *pixel, uint32_t *pixel_below, uint32_t bits) {
    for (uint32_t col = 0; col < FONT_WIDTH; col++) {
        if (bits & 0x80000000) {
            // Piksel utama
            pixel[col] = VGAAttr.foreground;

            // Tebalkan ke samping
            if (col + 1 < FONT_WIDTH) pixel[col + 1] = VGAAttr.foreground;
            if (col > 0)              pixel[col - 1] = VGAAttr.foreground;

            // Tebalkan ke bawah
            if (pixel_below) {
                pixel_below[col] = VGAAttr.foreground;
                if (col + 1 < FONT_WIDTH) pixel_below[col + 1] = VGAAttr.foreground;
                if (col > 0)              pixel_below[col - 1] = VGAAttr.foreground;
            }
        } else {
            pixel[col] = VGAAttr.background;
        }
        bits <<= 1;
    }
}

void clear_screen() {
   for(uint32_t iCount = 0 ; iCount < ((KernelVGA.window_width)*(KernelVGA.window_height)) ; iCount++)
      *(KernelVGA.frameBufPointer+iCount) = 0x00000000;
   return;
}

void draw_cursor() {
   memcpy(cursor_layer_buffer,font[95],sizeof(cursor_layer_buffer));
   KernelVGA.cursor_set = TRUE;
   for (uint32_t row = 0; row < FONT_HEIGHT; row++) {
      uint32_t bits = (font_layer_buffer[row] | cursor_layer_buffer[row]) << (32 - (sizeof(font[0][0]) << 3));

      // Hitung posisi piksel awal di framebuffer
      uint32_t *pixel = KernelVGA.frameBufPointer +
                         ((KernelVGA.cursor_y * FONT_HEIGHT + row) * KernelVGA.window_width) +
                         (KernelVGA.cursor_x * FONT_WIDTH + KernelVGA.leftrightmarginpiksel);

      uint32_t *pixel_below = (row + 1 < FONT_HEIGHT)
                               ? pixel + KernelVGA.window_width
                               : NULL;

      // Gambar per baris
      if (KernelVGA.bold) {
           draw_bold_glyph_row(KernelVGA, pixel, pixel_below, bits);
      } else {
           draw_normal_glyph_row(KernelVGA, pixel, bits);
      }
   }
}

void clear_cursor(){
   memset(cursor_layer_buffer, 0, sizeof(cursor_layer_buffer));
   KernelVGA.cursor_set = FALSE;
   for (uint32_t row = 0; row < FONT_HEIGHT; row++) {
      uint32_t bits = (font_layer_buffer[row] | cursor_layer_buffer[row]) << (32 - (sizeof(font[0][0]) << 3));

      // Hitung posisi piksel awal di framebuffer
      uint32_t *pixel = KernelVGA.frameBufPointer +
                         ((KernelVGA.cursor_y * FONT_HEIGHT + row) * KernelVGA.window_width) +
                         (KernelVGA.cursor_x * FONT_WIDTH + KernelVGA.leftrightmarginpiksel);

      uint32_t *pixel_below = (row + 1 < FONT_HEIGHT)
                               ? pixel + KernelVGA.window_width
                               : NULL;

      // Gambar per baris
      if (KernelVGA.bold) {
           draw_bold_glyph_row(KernelVGA, pixel, pixel_below, bits);
      } else {
           draw_normal_glyph_row(KernelVGA, pixel, bits);
      }
   }
}

void vga_set_cursor_timer(void *data){
   if (KernelVGA.cursor_set)
      clear_cursor();
   else
      draw_cursor();

   if (pit_add_event(300, vga_set_cursor_timer, NULL) == -1) {
      printf("\n VGA add PIT event error ! \n");
      // Tangani error: event timer tidak dapat ditambahkan
   }
}

void move_cursor(uint32_t y, uint32_t x) {
   if (x >= ((KernelVGA.window_width-KernelVGA.leftrightmarginpiksel)/FONT_WIDTH)) {
      x = 0;
      y++;
   }

   // Hilangkan cursor
   // Tiap karakter 16 baris tinggi
   for (uint32_t row = 0; row < FONT_HEIGHT; row++) {
      uint32_t bits = (font_layer_buffer[row]) << (32 - (sizeof(font[0][0]) << 3));

      // Hitung posisi piksel awal di framebuffer
      uint32_t *pixel = KernelVGA.frameBufPointer +
                         ((KernelVGA.cursor_y * FONT_HEIGHT + row) * KernelVGA.window_width) +
                         (KernelVGA.cursor_x * FONT_WIDTH + KernelVGA.leftrightmarginpiksel);

      uint32_t *pixel_below = (row + 1 < FONT_HEIGHT)
                               ? pixel + KernelVGA.window_width
                               : NULL;

      // Gambar per baris
      if (KernelVGA.bold) {
           draw_bold_glyph_row(font_layer_attribute, pixel, pixel_below, bits);
      } else {
           draw_normal_glyph_row(font_layer_attribute, pixel, bits);
      }
   }

   // Cek jika kursor melewati batas layar, lakukan scroll
   if ((y) >= (uint32_t)(KernelVGA.window_height/FONT_HEIGHT)) {

      // Jumlah baris piksel yang akan di-scroll (satu baris teks)
      uint32_t scroll_pixels = FONT_HEIGHT;
      uint32_t pixels_to_move = KernelVGA.window_width * (KernelVGA.window_height - scroll_pixels);

      // Jika SSE2 tersedia dan jumlah pixel yang akan dipindahkan cukup (minimal 4 pixel per blok 16 byte)
      if (g_has_sse2 && (pixels_to_move >= 4)) {
          // Karena setiap pixel 4 byte, sebaris 16 byte = 4 pixel.
          uint32_t blocks = pixels_to_move / 4;
          uint32_t remainder = pixels_to_move % 4;
          // Siapkan pointer untuk operasi berbasis byte
          uint8_t *dest_ptr = (uint8_t *)KernelVGA.frameBufPointer+(KernelVGA.totalPixel<<2);
          // Geser offset sumber: jumlah pixel yang diskip x 4 byte plus offset awal
          uint8_t *src_ptr = (uint8_t *)KernelVGA.frameBufPointer + (scroll_pixels * KernelVGA.window_width * 4);

          asm volatile (
              "1:\n\t"
              "movdqu   (%1), %%xmm0\n\t"  /* Load 16 byte (4 pixel) dari src_ptr ke xmm0 */
              "movdqu   %%xmm0, (%0)\n\t"  /* Simpan 16 byte dari xmm0 ke dest_ptr */
              "add      $16, %0\n\t"       /* dest_ptr += 16 */
              "add      $16, %1\n\t"       /* src_ptr += 16 */
              "dec      %2\n\t"            /* block counter-- */
              "jnz      1b\n\t"
              : "+r" (dest_ptr), "+r" (src_ptr), "+r" (blocks)
              :
              : "xmm0", "memory"
          );

          // Copy sisa pixel yang tidak kelipatan 4
          for (uint32_t i = 0; i < remainder; i++) {
              *((uint32_t *)dest_ptr) = *((uint32_t *)src_ptr);
              dest_ptr += 4;
              src_ptr += 4;
          }
      } else {
          // Fallback: loop biasa
          for (uint32_t i = 0; i < pixels_to_move; i++) {
              (KernelVGA.frameBufPointer+KernelVGA.totalPixel)[i] = KernelVGA.frameBufPointer[i + (scroll_pixels * KernelVGA.window_width)];
          }
      }

      // Kosongkan baris terakhir (background)
      for (uint32_t i = pixels_to_move; i < KernelVGA.totalPixel; i++) {
         (KernelVGA.frameBufPointer+KernelVGA.totalPixel)[i] = KernelVGA.background; // background color
      }

      // Setelah scroll, kursor berada di baris terakhir
      y = (y_resolution / FONT_HEIGHT) - 1;
      memcpy(KernelVGA.frameBufPointer, KernelVGA.frameBufPointer+KernelVGA.totalPixel, KernelVGA.totalPixel<<2);
   }

   memset(font_layer_buffer,0,sizeof(font_layer_buffer));
   KernelVGA.cursor_x = x;
   KernelVGA.cursor_y = y;
}

static inline void draw_glyph_row(uint32_t *pixel, uint32_t bits) {
    for (uint32_t col = 0; col < FONT_WIDTH; col++) {
        *pixel = (bits & 0x80000000) ? KernelVGA.foreground : KernelVGA.background;
        pixel++;
        bits <<= 1;
    }
}

void write_font(uint8_t input_char) {
    if (filter_escape_char(&KernelVGA, input_char) == -1)
        return;

    switch (input_char) {
        case '\n':
            move_cursor(KernelVGA.cursor_y + 1, 0);
            break;
        case '\r':
            move_cursor(KernelVGA.cursor_y, 0);
            break;
        default:
            // Salin glyph karakter dari font array
            memcpy(font_layer_buffer, font[input_char - 32], sizeof(font_layer_buffer));
            font_layer_attribute = KernelVGA;

            for (uint32_t row = 0; row < FONT_HEIGHT; row++) {
                uint32_t bits = (font_layer_buffer[row] | cursor_layer_buffer[row]) << (32 - (sizeof(font[0][0]) << 3));

                // Hitung posisi piksel awal di framebuffer
                uint32_t *pixel = KernelVGA.frameBufPointer +
                                  ((KernelVGA.cursor_y * FONT_HEIGHT + row) * KernelVGA.window_width) +
                                  (KernelVGA.cursor_x * FONT_WIDTH + KernelVGA.leftrightmarginpiksel);

                uint32_t *pixel_below = (row + 1 < FONT_HEIGHT)
                                        ? pixel + KernelVGA.window_width
                                        : NULL;

                // Gambar per baris
                if (KernelVGA.bold) {
                    draw_bold_glyph_row(font_layer_attribute, pixel, pixel_below, bits);
                } else {
                    draw_normal_glyph_row(font_layer_attribute, pixel, bits);
                }
            }

            move_cursor(KernelVGA.cursor_y, KernelVGA.cursor_x + 1);
            break;
    }
    draw_cursor();
}
