#ifndef __TIMERPIT_H
#define __TIMERPIT_H

#include <types.h>

/* Definisi port untuk PIT */
#define PIT_CHANNEL0      0x40
#define PIT_COMMAND       0x43

/* PIT memiliki clock input 1,193,182 Hz */
#define PIT_BASE_FREQUENCY 1193182

/* Tipe fungsi callback untuk event timer */
typedef void (*timer_callback_t)(void *);

/* Struktur event timer */
typedef struct {
    uint32_t trigger_time;         // Waktu eksekusi event (dalam ms)
    timer_callback_t callback;     // Fungsi yang akan dipanggil saat event jatuh tempo
    void *data;                    // Data untuk callback
    uint8_t used;                  // 0 = slot kosong, 1 = terpakai
} timer_event_t;

// Declaration
void initPIT(uint32_t frequency);
void pitHandler(void );
int32_t pit_add_event(uint32_t delay_ms, timer_callback_t callback, void *data);
uint32_t pit_get_ms(); 

#endif            //__TIMERPIT_H
