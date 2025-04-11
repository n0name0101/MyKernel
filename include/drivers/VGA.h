#ifndef __VGA_H
#define __VGA_H

#include <boolean.h>
#include <types.h>

void initVGA();
void clear_screen();
void write_font(uint8_t );
void move_cursor(uint32_t y, uint32_t x);
void vga_set_cursor_timer(void *);

typedef struct {
   uint16_t window_width;           // Width
   uint16_t window_height;          // Height
   uint32_t totalPixel;
   uint32_t leftrightmarginpiksel;
   uint32_t cursor_x;
   uint32_t cursor_y ;
   bool     bold;
   uint32_t foreground;
   uint32_t background;
   bool cursor_set;

   uint32_t* frameBufPointer;
}VGA;

#endif            // __VGA_H
